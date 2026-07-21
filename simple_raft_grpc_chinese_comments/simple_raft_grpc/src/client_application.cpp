#include "simple_raft/client_application.h"

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "raft.grpc.pb.h"

namespace simple_raft {

int ClientApplication::Run(int argc, char** argv) {
  // 必须至少指定目标地址和命令，例如：raft_client 127.0.0.1:50051 status。
  if (argc < 3) {
    std::cerr << "Usage: raft_client <host:port> <status|elect|propose> [key=value]\n";
    return 2;
  }

  // 一个客户端进程只操作一个目标节点；若目标不是 Leader，propose 会得到拒绝结果。
  auto channel = grpc::CreateChannel(argv[1], grpc::InsecureChannelCredentials());
  std::unique_ptr<rpc::RaftService::Stub> stub = rpc::RaftService::NewStub(channel);
  const std::string command = argv[2];

  if (command == "status") {
    // 查询节点角色、任期、日志进度和提交进度。
    rpc::StatusRequest request;
    rpc::StatusResponse response;
    grpc::ClientContext context;
    const grpc::Status status = stub->GetStatus(&context, request, &response);
    if (!status.ok()) {
      std::cerr << status.error_message() << '\n';
      return 1;
    }
    std::cout << "node=" << response.node_id() << " role=" << response.role()
              << " term=" << response.current_term() << " leader=" << response.leader_id()
              << " last_log=" << response.last_log_index()
              << " commit=" << response.commit_index()
              << " applied=" << response.last_applied() << '\n';
    return 0;
  }

  if (command == "elect") {
    // 演示用手动选举；真实系统应由选举超时自动调用相同业务流程。
    rpc::StartElectionRequest request;
    rpc::StartElectionResponse response;
    grpc::ClientContext context;
    const grpc::Status status = stub->StartElection(&context, request, &response);
    if (!status.ok()) {
      std::cerr << status.error_message() << '\n';
      return 1;
    }
    std::cout << "term=" << response.term() << " votes=" << response.votes_received()
              << " leader=" << response.became_leader() << " role=" << response.role()
              << '\n';
    return 0;
  }

  if (command == "propose" && argc >= 4) {
    // 提交 key=value 命令；只有 Leader 在得到多数派复制确认后才返回 accepted=true。
    rpc::CommandRequest request;
    request.set_command(argv[3]);
    rpc::CommandResponse response;
    grpc::ClientContext context;
    const grpc::Status status = stub->Propose(&context, request, &response);
    if (!status.ok()) {
      std::cerr << status.error_message() << '\n';
      return 1;
    }
    std::cout << "accepted=" << response.accepted() << " message=" << response.message()
              << " log_index=" << response.log_index()
              << " commit_index=" << response.commit_index() << '\n';
    return response.accepted() ? 0 : 1;
  }

  std::cerr << "Unsupported command\n";
  return 2;
}

}  // namespace simple_raft
