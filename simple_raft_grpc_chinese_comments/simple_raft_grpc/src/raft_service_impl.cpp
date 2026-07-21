#include "simple_raft/raft_service_impl.h"

#include "simple_raft/types.h"

namespace simple_raft {

RaftServiceImpl::RaftServiceImpl(RaftNode& raft_node) : raft_node_(raft_node) {}

grpc::Status RaftServiceImpl::RequestVote(grpc::ServerContext* context,
                                          const rpc::VoteRequest* request,
                                          rpc::VoteResponse* response) {
  (void)context;  // 本示例不读取客户端元数据；保留参数以符合 gRPC 重写签名。

  // protobuf -> 内部模型 -> Raft 业务层 -> protobuf 响应。
  const VoteResponseData result = raft_node_.HandleVoteRequest(
      VoteRequestData{request->term(), request->candidate_id(), request->last_log_index(),
                      request->last_log_term()});
  response->set_term(result.term);
  response->set_vote_granted(result.vote_granted);
  return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::AppendEntries(grpc::ServerContext* context,
                                            const rpc::AppendEntriesRequest* request,
                                            rpc::AppendEntriesResponse* response) {
  (void)context;

  // 1. 先复制基础字段；entries 需要从 repeated protobuf 字段逐条转换。
  AppendEntriesRequestData internal_request{request->term(),
                                            request->leader_id(),
                                            request->prev_log_index(),
                                            request->prev_log_term(),
                                            request->leader_commit(),
                                            {}};
  internal_request.entries.reserve(static_cast<std::size_t>(request->entries_size()));
  for (const rpc::LogEntry& entry : request->entries()) {
    internal_request.entries.push_back(LogEntryData{entry.index(), entry.term(), entry.command()});
  }

  // 2. 业务层处理任期、前序日志校验、冲突截断、追加及提交推进。
  const AppendResultData result = raft_node_.HandleAppendEntries(internal_request);
  response->set_term(result.term);
  response->set_success(result.success);
  response->set_match_index(result.match_index);
  return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::GetStatus(grpc::ServerContext* context,
                                        const rpc::StatusRequest* request,
                                        rpc::StatusResponse* response) {
  (void)context;
  (void)request;

  const StatusData status = raft_node_.GetStatus();
  response->set_node_id(status.node_id);
  response->set_role(ToString(status.role));
  response->set_current_term(status.current_term);
  response->set_leader_id(status.leader_id);
  response->set_last_log_index(status.last_log_index);
  response->set_commit_index(status.commit_index);
  response->set_last_applied(status.last_applied);
  return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::StartElection(
    grpc::ServerContext* context, const rpc::StartElectionRequest* request,
    rpc::StartElectionResponse* response) {
  (void)context;
  (void)request;

  // 演示入口：真实生产系统应由随机化 election timeout 自动触发。
  const ElectionResultData result = raft_node_.StartElection();
  response->set_term(result.term);
  response->set_became_leader(result.became_leader);
  response->set_votes_received(result.votes_received);
  response->set_role(ToString(result.role));
  return grpc::Status::OK;
}

grpc::Status RaftServiceImpl::Propose(grpc::ServerContext* context,
                                      const rpc::CommandRequest* request,
                                      rpc::CommandResponse* response) {
  (void)context;

  // RaftNode 负责判断是否为 Leader、落盘、复制、达到多数派和提交。
  const CommandResultData result = raft_node_.ProposeCommand(request->command());
  response->set_accepted(result.accepted);
  response->set_message(result.message);
  response->set_log_index(result.log_index);
  response->set_commit_index(result.commit_index);
  return grpc::Status::OK;
}

}  // namespace simple_raft
