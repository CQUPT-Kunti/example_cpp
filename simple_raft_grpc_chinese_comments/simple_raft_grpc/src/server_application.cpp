#include "simple_raft/server_application.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <grpcpp/grpcpp.h>

#include "simple_raft/raft_node.h"
#include "simple_raft/raft_service_impl.h"
#include "simple_raft/types.h"

namespace simple_raft {

int ServerApplication::Run(int argc, char** argv) {
  // 默认配置用于快速单节点演示；命令行参数可以覆盖其中任何字段。
  NodeConfig config{"node-1", "0.0.0.0:50051", "./data/node-1", {}};

  // 支持多次 --peer，每个 Peer 使用 id=host:port 格式。
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--id" && index + 1 < argc) {
      config.node_id = argv[++index];
    } else if (argument == "--listen" && index + 1 < argc) {
      config.listen_address = argv[++index];
    } else if (argument == "--data" && index + 1 < argc) {
      config.data_directory = argv[++index];
    } else if (argument == "--peer" && index + 1 < argc) {
      const std::string peer_argument = argv[++index];
      const std::size_t separator = peer_argument.find('=');
      if (separator == std::string::npos) {
        std::cerr << "Peer format must be id=host:port\n";
        return 2;
      }

      const std::string peer_id = peer_argument.substr(0, separator);
      const std::string peer_address = peer_argument.substr(separator + 1);
      config.peers.push_back(PeerEndpoint{peer_id, peer_address});
    } else if (argument == "--help") {
      std::cout << "Usage: raft_server [--id node-1] [--listen 0.0.0.0:50051] "
                   "[--data ./data/node-1] [--peer node-2=127.0.0.1:50052]\n";
      return 0;
    } else {
      std::cerr << "Unknown argument: " << argument << '\n';
      return 2;
    }
  }

  // 1. 创建并恢复 Raft 节点；Initialize 会加载磁盘日志并回放已提交数据。
  RaftNode raft_node(config);
  raft_node.Initialize();

  // 2. 将 gRPC 服务适配器注册给 gRPC Server。
  RaftServiceImpl service(raft_node);
  grpc::ServerBuilder builder;
  builder.AddListeningPort(config.listen_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  if (!server) {
    std::cerr << "Failed to start gRPC server on " << config.listen_address << '\n';
    return 1;
  }

  // 3. Wait 会阻塞当前主线程，直到服务被进程信号或外部代码关闭。
  std::cout << "Raft node " << config.node_id << " listening on " << config.listen_address
            << '\n';
  server->Wait();
  return 0;
}

}  // namespace simple_raft
