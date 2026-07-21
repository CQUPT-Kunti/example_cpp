#include "simple_raft/peer_replicator.h"

#include <chrono>
#include <memory>
#include <utility>

#include <grpcpp/grpcpp.h>

#include "raft.grpc.pb.h"

namespace simple_raft {

PeerReplicator::PeerReplicator(std::vector<PeerEndpoint> peers) : peers_(std::move(peers)) {}

std::vector<VoteResponseData> PeerReplicator::RequestVotes(
    const VoteRequestData& request) const {
  std::vector<VoteResponseData> responses;
  responses.reserve(peers_.size());

  // 逐个请求对端投票。教学实现采用同步串行 RPC，便于理解；生产可并行化。
  for (const PeerEndpoint& peer : peers_) {
    responses.push_back(RequestVoteFromPeer(peer, request));
  }
  return responses;
}

std::size_t PeerReplicator::Replicate(const AppendEntriesRequestData& request) const {
  std::size_t successful_replicas = 0;

  // 只统计明确返回 success=true 的节点；网络超时和日志冲突都不能计入多数派。
  for (const PeerEndpoint& peer : peers_) {
    const AppendResultData response = SendAppendToPeer(peer, request);
    if (response.success) {
      ++successful_replicas;
    }
  }
  return successful_replicas;
}

void PeerReplicator::BroadcastCommit(const AppendEntriesRequestData& request) const {
  // 日志达到多数派后，Leader 再广播新的 leader_commit，使 Follower 应用同一批已提交日志。
  // 此处不统计结果；后续复制时仍会继续携带 leader_commit 作为补偿。
  for (const PeerEndpoint& peer : peers_) {
    SendAppendToPeer(peer, request);
  }0
}

VoteResponseData PeerReplicator::RequestVoteFromPeer(
    const PeerEndpoint& peer, const VoteRequestData& request) const {
  // 1. 建立到单个 Peer 的 gRPC Channel 和 Stub。
  auto channel = grpc::CreateChannel(peer.address, grpc::InsecureChannelCredentials());
  std::unique_ptr<rpc::RaftService::Stub> stub = rpc::RaftService::NewStub(channel);

  // 2. 将内部数据模型转换为 protobuf。
  rpc::VoteRequest rpc_request;
  rpc_request.set_term(request.term);
  rpc_request.set_candidate_id(request.candidate_id);
  rpc_request.set_last_log_index(request.last_log_index);
  rpc_request.set_last_log_term(request.last_log_term);

  // 3. 发送同步 RPC，并设置较短超时，避免故障节点无限阻塞一次选举。
  rpc::VoteResponse rpc_response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(800));
  const grpc::Status status = stub->RequestVote(&context, rpc_request, &rpc_response);
  if (!status.ok()) {
    // term=0 是本示例的“没有拿到有效响应”占位值，不会被当作高任期处理。
    return VoteResponseData{0, false};
  }

  // 4. 转回内部模型，交由 RaftNode 汇总票数和高任期信息。
  return VoteResponseData{rpc_response.term(), rpc_response.vote_granted()};
}

AppendResultData PeerReplicator::SendAppendToPeer(
    const PeerEndpoint& peer, const AppendEntriesRequestData& request) const {
  // 1. 连接目标 Follower。
  auto channel = grpc::CreateChannel(peer.address, grpc::InsecureChannelCredentials());
  std::unique_ptr<rpc::RaftService::Stub> stub = rpc::RaftService::NewStub(channel);

  // 2. 将 Leader 任期、前序日志锚点、提交位点和日志批次填充到 protobuf。
  rpc::AppendEntriesRequest rpc_request;
  rpc_request.set_term(request.term);
  rpc_request.set_leader_id(request.leader_id);
  rpc_request.set_prev_log_index(request.prev_log_index);
  rpc_request.set_prev_log_term(request.prev_log_term);
  rpc_request.set_leader_commit(request.leader_commit);
  for (const LogEntryData& entry : request.entries) {
    rpc::LogEntry* rpc_entry = rpc_request.add_entries();
    rpc_entry->set_index(entry.index);
    rpc_entry->set_term(entry.term);
    rpc_entry->set_command(entry.command);
  }

  // 3. 发送 RPC。网络错误与业务失败均由返回值交给上层决定后续策略。
  rpc::AppendEntriesResponse rpc_response;
  grpc::ClientContext context;
  context.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(800));
  const grpc::Status status = stub->AppendEntries(&context, rpc_request, &rpc_response);
  if (!status.ok()) {
    return AppendResultData{0, false, 0};
  }

  return AppendResultData{rpc_response.term(), rpc_response.success(), rpc_response.match_index()};
}

}  // namespace simple_raft
