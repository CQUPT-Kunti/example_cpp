#pragma once

#include <cstddef>
#include <vector>

#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 通过 Google gRPC 向其他成员发送 Raft RPC。
 *
 * 该类只负责序列化、网络调用和响应转换，不决定何时选举、何时提交或如何回退日志。
 */
class PeerReplicator {
 public:
  /** @param peers 当前集群中除本节点以外的远端成员。 */
  explicit PeerReplicator(std::vector<PeerEndpoint> peers);

  /**
   * @brief 向全部对端广播 RequestVote。
   * @param request 已由 ElectionManager 创建的投票请求。
   * @return 每个对端对应一个响应；网络失败会返回 term=0、vote_granted=false 的占位结果。
   */
  std::vector<VoteResponseData> RequestVotes(const VoteRequestData& request) const;

  /**
   * @brief 向全部对端发送日志复制请求，并计数成功副本数。
   * @param request AppendEntries 请求，可包含日志或只是提交通知。
   * @return 返回 success=true 的远端节点数量，不包含本节点。
   */
  std::size_t Replicate(const AppendEntriesRequestData& request) const;

  /**
   * @brief 向全部对端广播新的 leader_commit。
   * @param request 通常 entries 为空，仅携带已确认的 commit_index。
   */
  void BroadcastCommit(const AppendEntriesRequestData& request) const;

 private:
  /**
   * @brief 对单个远端节点调用 RequestVote，包含 800ms 演示级超时。
   */
  VoteResponseData RequestVoteFromPeer(const PeerEndpoint& peer,
                                       const VoteRequestData& request) const;

  /**
   * @brief 对单个远端节点调用 AppendEntries，包含 800ms 演示级超时。
   */
  AppendResultData SendAppendToPeer(const PeerEndpoint& peer,
                                    const AppendEntriesRequestData& request) const;

  std::vector<PeerEndpoint> peers_;  ///< 远端成员静态列表。
};

}  // namespace simple_raft
