#include "simple_raft/raft_node.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace simple_raft
{

  RaftNode::RaftNode(NodeConfig config)
      : config_(std::move(config)),
        log_store_(config_.data_directory),
        election_manager_(config_.node_id, log_store_),
        log_replicator_(log_store_),
        commit_manager_(log_store_, state_machine_),
        catch_up_manager_(log_store_),
        peer_replicator_(config_.peers) {}

  void RaftNode::Initialize()
  {
    // 节点启动的恢复顺序十分重要：先恢复持久化日志和提交点，再回放到内存状态机。
    log_store_.Load();
    commit_manager_.Recover();
  }

  VoteResponseData RaftNode::HandleVoteRequest(const VoteRequestData &request)
  {
    // 投票规则集中在 ElectionManager，RaftNode 不重复实现。
    return election_manager_.HandleVoteRequest(request);
  }

  AppendResultData RaftNode::HandleAppendEntries(const AppendEntriesRequestData &request)
  {
    // 1. 先检查 Leader 任期。旧任期 Leader 必须被拒绝；高/同任期 Leader 会让本节点成为 Follower。
    if (!election_manager_.ObserveLeader(request.term, request.leader_id))
    {
      return AppendResultData{election_manager_.CurrentTerm(), false, log_store_.LastIndex()};
    }

    // 2. 通过 prev_log 检查、冲突截断、追加缺失日志，确保本地日志追随 Leader。
    AppendResultData result = log_replicator_.AppendFromLeader(request);
    if (!result.success)
    {
      return result;
    }

    // 3. 仅应用 Leader 已经确认的部分。Follower 取 min(leader_commit, local_last_index)。
    const std::uint64_t commit_target = std::min(request.leader_commit, log_store_.LastIndex());
    commit_manager_.AdvanceCommit(commit_target);

    // 4. 回包携带更新后的任期和本地最大匹配日志序号。
    result.term = election_manager_.CurrentTerm();
    result.match_index = log_store_.LastIndex();
    return result;
  }

  ElectionResultData RaftNode::StartElection()
  {
    // 1. 本地进入新任期 Candidate 状态，并得到包含自身末尾日志的 RequestVote。
    const VoteRequestData request = election_manager_.StartElection();

    // 2. 自己先投票给自己；对端响应将由 PeerReplicator 收集。
    std::uint64_t votes_received = 1;
    const std::vector<VoteResponseData> responses = peer_replicator_.RequestVotes(request);
    for (const VoteResponseData &response : responses)
    {
      if (response.term > request.term)
      {
        // 任一 Peer 告知更高任期，当前选举立刻失效，本节点必须退回 Follower。
        election_manager_.StepDown(response.term);
        return ElectionResultData{response.term, false, votes_received, election_manager_.Role()};
      }
      if (response.vote_granted)
      {
        ++votes_received;
      }
    }

    // 3. 多数票成功才会真正成为 Leader；否则仍保留 Candidate，等待外部下一次触发。
    const bool became_leader = election_manager_.CompleteElection(votes_received, QuorumSize());
    return ElectionResultData{election_manager_.CurrentTerm(), became_leader, votes_received,
                              election_manager_.Role()};
  }

  CommandResultData RaftNode::ProposeCommand(const std::string &command)
  {
    // 1. Raft 只允许 Leader 接收客户端写入；Follower 不能自行创建日志。
    if (election_manager_.Role() != NodeRole::kLeader)
    {
      return CommandResultData{false, "node is not leader", 0, commit_manager_.CommitIndex()};
    }

    // 2. Leader 将命令追加到本地日志并落盘。此时它仍是“未提交”的候选日志。
    const LogEntryData entry{log_store_.LastIndex() + 1, election_manager_.CurrentTerm(), command};
    if (!log_store_.Append(entry))
    {
      return CommandResultData{false, "failed to persist the log entry", 0,
                               commit_manager_.CommitIndex()};
    }

    // 3. 向其他节点复制日志。本教学版本发送全量日志，以清晰展示追赶与冲突修复过程。
    AppendEntriesRequestData replication_request =
        BuildFullReplicationRequest(commit_manager_.CommitIndex());
    const std::size_t acknowledged_peers = peer_replicator_.Replicate(replication_request);
    const std::uint64_t replicated_nodes = static_cast<std::uint64_t>(acknowledged_peers + 1);
    if (replicated_nodes < QuorumSize())
    {
      // 日志已经本地持久化，但未到多数派，不能应用到状态机，也不能向客户端报告成功。
      return CommandResultData{false, "log persisted but did not reach quorum", entry.index,
                               commit_manager_.CommitIndex()};
    }

    // 4. 多数派确认后推进 commit_index，并将这条日志应用到本地状态机。
    commit_manager_.AdvanceCommit(entry.index);

    // 5. 再发送一个不携带 entries 的 AppendEntries，把新的 leader_commit 通知给 Follower。
    AppendEntriesRequestData commit_notification =
        BuildFullReplicationRequest(commit_manager_.CommitIndex());
    commit_notification.entries.clear();
    commit_notification.prev_log_index = log_store_.LastIndex();
    commit_notification.prev_log_term = log_store_.LastTerm();
    peer_replicator_.BroadcastCommit(commit_notification);

    return CommandResultData{true, "command committed", entry.index, commit_manager_.CommitIndex()};
  }

  StatusData RaftNode::GetStatus() const
  {
    // 汇总各模块状态。每个模块内部负责各自字段的并发保护。
    return StatusData{config_.node_id,
                      election_manager_.Role(),
                      election_manager_.CurrentTerm(),
                      election_manager_.LeaderId(),
                      log_store_.LastIndex(),
                      commit_manager_.CommitIndex(),
                      commit_manager_.LastApplied()};
  }

  std::uint64_t RaftNode::QuorumSize() const
  {
    // 集群总节点数 = 远端 peers 数 + 本节点；多数派规则为 floor(N / 2) + 1。
    const std::uint64_t cluster_size = static_cast<std::uint64_t>(config_.peers.size() + 1);
    return cluster_size / 2 + 1;
  }

  AppendEntriesRequestData RaftNode::BuildFullReplicationRequest(
      std::uint64_t leader_commit) const
  {
    // 本示例的简化点：prev_log_index=0，并发送所有日志。
    // 正式 Raft 应按每个 Peer 的 nextIndex 计算 prev_log_index，并发送有限大小的增量批次。
    return AppendEntriesRequestData{election_manager_.CurrentTerm(),
                                    config_.node_id,
                                    0,
                                    0,
                                    leader_commit,
                                    catch_up_manager_.BuildCatchUpBatch(
                                        0, std::numeric_limits<std::size_t>::max())};
  }

} // namespace simple_raft
