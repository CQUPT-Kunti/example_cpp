#pragma once

#include <cstdint>
#include <mutex>
#include <string>

#include "simple_raft/log_store.h"
#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 负责 Raft 的任期管理、投票规则、角色切换和已知 Leader 记录。
 *
 * 该类不负责网络发送；RaftNode 收集网络投票结果后，再调用 CompleteElection。
 */
class ElectionManager {
 public:
  /**
   * @param local_node_id 本节点 ID，用作候选人 ID 和 Leader ID。
   * @param log_store 任期与 voted_for 的持久化载体，生命周期须长于本对象。
   */
  ElectionManager(std::string local_node_id, LogStore& log_store);

  /**
   * @brief 在本节点发起新选举，并生成 RequestVote 请求。
   *
   * 流程：任期加一 -> 投票给自己 -> 角色变为 Candidate -> 清空旧 Leader ->
   * 取本地末尾日志信息，供其他节点判定日志是否足够新。
   */
  VoteRequestData StartElection();

  /**
   * @brief 按 Raft 投票规则处理一个远端候选人的请求。
   * @param request 含候选人任期、ID 和末尾日志位置的投票请求。
   * @return 响应当前任期与是否授予投票。
   */
  VoteResponseData HandleVoteRequest(const VoteRequestData& request);

  /**
   * @brief 观察到一个 Leader 的 AppendEntries 后，更新本地任期和角色。
   * @param leader_term 请求中携带的 Leader 任期。
   * @param leader_id 请求中的 Leader ID。
   * @return true 表示任期不落后、可继续处理该 Leader 的日志；false 表示应拒绝。
   */
  bool ObserveLeader(std::uint64_t leader_term, const std::string& leader_id);

  /**
   * @brief 在 RaftNode 收集完投票后，判定候选人是否晋升 Leader。
   * @param votes_received 已获得票数，必须含自身的一票。
   * @param quorum_size 集群多数派门槛，例如 3 节点集群为 2。
   * @return true 表示本节点从 Candidate 转为 Leader。
   */
  bool CompleteElection(std::uint64_t votes_received, std::uint64_t quorum_size);

  /**
   * @brief 发现更高任期后降级为 Follower。
   * @param new_term 远端返回的更高任期；若不高于本地任期，仅执行角色降级。
   */
  void StepDown(std::uint64_t new_term);

  /** @brief 返回当前节点角色。 */
  NodeRole Role() const;

  /** @brief 返回已持久化的当前任期。 */
  std::uint64_t CurrentTerm() const;

  /** @brief 返回本节点当前已知 Leader ID；未知时为空字符串。 */
  std::string LeaderId() const;

 private:
  /**
   * @brief 判断候选人日志是否“至少和本地一样新”。
   *
   * 先比较最后日志任期；任期相同才比较最后日志序号。这是 Raft 的投票安全条件。
   */
  bool IsCandidateLogUpToDate(const VoteRequestData& request) const;

  std::string local_node_id_;   ///< 本节点 ID。
  LogStore& log_store_;         ///< 任期、投票和日志元数据存储。
  NodeRole role_;               ///< 内存中的实时角色。
  std::string leader_id_;       ///< 最近接受的 Leader ID。
  mutable std::mutex mutex_;    ///< 保护角色与 leader_id_ 的复合更新。
};

}  // namespace simple_raft
