#pragma once

namespace simple_raft {

/** @brief 解析服务端命令行参数、初始化 RaftNode 并启动 gRPC Server。 */
class ServerApplication {
 public:
  /**
   * @brief 服务端程序入口逻辑。
   * @param argc 命令行参数数量。
   * @param argv 命令行参数数组，支持 --id、--listen、--data、--peer。
   * @return 0 表示正常退出；非 0 表示参数或服务启动失败。
   */
  static int Run(int argc, char** argv);
};

}  // namespace simple_raft
