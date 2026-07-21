#pragma once

namespace simple_raft {

/** @brief 演示用 gRPC 客户端，提供 status、elect、propose 三类命令。 */
class ClientApplication {
 public:
  /**
   * @brief 解析命令行并调用目标 Raft 节点的 gRPC 接口。
   * @param argc 命令行参数数量。
   * @param argv 命令行参数数组，格式为 raft_client <host:port> <command> [key=value]。
   * @return 0 表示 RPC 成功；非 0 表示参数错误、网络失败或命令未提交。
   */
  static int Run(int argc, char** argv);
};

}  // namespace simple_raft
