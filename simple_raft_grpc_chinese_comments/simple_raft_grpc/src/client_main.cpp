#include "simple_raft/client_application.h"

int main(int argc, char** argv) {
  // main 仅保留 C++ 程序入口；参数解析和 RPC 逻辑在 client_application.cpp。
  return simple_raft::ClientApplication::Run(argc, argv);
}
