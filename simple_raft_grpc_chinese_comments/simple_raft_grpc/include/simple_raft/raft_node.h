#pragma once

#include <memory>

#include "simple_raft/catch_up_manager.h"
#include "simple_raft/commit_manager.h"
#include "simple_raft/election_manager.h"
#include "simple_raft/log_replicator.h"
#include "simple_raft/peer_replicator.h"
#include "simple_raft/state_machine.h"
#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 一个 Raft 节点的业务编排入口。
 *
 * RaftNode 将选举、日志持久化、日志校验/复制、提交管理、节点追赶和 gRPC 网络调用
 * 串为完整链路。gRPC Service 仅负责把 protobuf 与本类的内部结构互相转换。
 */
class RaftNode {
 public:
  /** @param config 本节点 ID、监听地址、存储路径和对端成员配置。 */
  explicit RaftNode(NodeConfig config);

  /**
   * @brief 初始化节点持久化状态。
   * 流程：LogStore 从磁盘恢复 -> CommitManager 回放所有已提交日志到状态机。
   */
  void Initialize();

  /**
   * @brief 处理其他节点发来的 RequestVote。
   * @param request 候选人请求。
   * @return 是否投票与本节点当前任期。
   */
  VoteResponseData HandleVoteRequest(const VoteRequestData& request);

  /**
   * @brief 处理 Leader 发来的 AppendEntries。
   * @param request 包含任期、前序日志、日志批次和 leader_commit。
   * @return 是否完成日志一致性校验/追加，以及当前匹配位置。
   */
  AppendResultData HandleAppendEntries(const AppendEntriesRequestData& request);

  /**
   * @brief 手动发起一次选举。
   *
   * 流程：成为 Candidate 并自投 -> gRPC 请求其他节点 -> 发现高任期立即降级 ->
   * 票数达到多数派则成为 Leader。
   */
  ElectionResultData StartElection();

  /**
   * @brief 接受客户端命令并尝试提交。
   * @param command 教学状态机使用 key=value 格式。
   * @return 若已持久化并复制到多数派则 accepted=true；否则本地日志可能仍存在但未提交。
   */
  CommandResultData ProposeCommand(const std::string& command);

  /** @brief 聚合当前节点的选举、日志和提交状态。 */
  StatusData GetStatus() const;

 private:
  /** @brief 计算集群多数派门槛：cluster_size / 2 + 1。 */
  std::uint64_t QuorumSize() const;

  /**
   * @brief 构造教学用全量日志复制请求。
   * @param leader_commit 要随请求传播给 Follower 的提交位点。
   * @return prev_log_index=0 的 AppendEntries，请求携带本地全部日志。
   */
  AppendEntriesRequestData BuildFullReplicationRequest(std::uint64_t leader_commit) const;

  NodeConfig config_;                  ///< 本节点静态配置。
  LogStore log_store_;                 ///< 持久化任期、投票、日志和提交位点。
  ElectionManager election_manager_;   ///< 选举/角色/任期管理。
  LogReplicator log_replicator_;       ///< Follower 侧日志一致性处理。
  StateMachine state_machine_;         ///< 已提交命令的教学状态机。
  CommitManager commit_manager_;       ///< commit_index 推进和状态机回放。
  CatchUpManager catch_up_manager_;    ///< 向落后节点组装日志批次。
  PeerReplicator peer_replicator_;     ///< 对端 gRPC 通信。
};

}  // namespace simple_raft
