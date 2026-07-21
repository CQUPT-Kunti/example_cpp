#pragma once

#include <grpcpp/grpcpp.h>

#include "raft.grpc.pb.h"
#include "simple_raft/raft_node.h"

namespace simple_raft {

/**
 * @brief gRPC 服务适配器。
 *
 * 所有 Raft 业务决策留在 RaftNode；本类只负责 protobuf 请求/响应与内部数据结构之间
 * 的转换，便于网络层与领域逻辑解耦。
 */
class RaftServiceImpl final : public rpc::RaftService::Service {
 public:
  /** @param raft_node 处理本机 Raft 业务的节点对象，必须在服务存活期内有效。 */
  explicit RaftServiceImpl(RaftNode& raft_node);

  /** @brief 接收其他节点的投票请求，并转交 RaftNode::HandleVoteRequest。 */
  grpc::Status RequestVote(grpc::ServerContext* context,
                           const rpc::VoteRequest* request,
                           rpc::VoteResponse* response) override;

  /** @brief 接收 Leader 的日志复制/提交通知，并转交 RaftNode::HandleAppendEntries。 */
  grpc::Status AppendEntries(grpc::ServerContext* context,
                             const rpc::AppendEntriesRequest* request,
                             rpc::AppendEntriesResponse* response) override;

  /** @brief 返回节点当前任期、角色、日志和提交进度。 */
  grpc::Status GetStatus(grpc::ServerContext* context,
                         const rpc::StatusRequest* request,
                         rpc::StatusResponse* response) override;

  /** @brief 演示接口：手工触发本节点开始一次选举。 */
  grpc::Status StartElection(grpc::ServerContext* context,
                             const rpc::StartElectionRequest* request,
                             rpc::StartElectionResponse* response) override;

  /** @brief 演示接口：向 Leader 提交一条 key=value 命令。 */
  grpc::Status Propose(grpc::ServerContext* context,
                       const rpc::CommandRequest* request,
                       rpc::CommandResponse* response) override;

 private:
  RaftNode& raft_node_;  ///< 不拥有对象，仅转发调用。
};

}  // namespace simple_raft
