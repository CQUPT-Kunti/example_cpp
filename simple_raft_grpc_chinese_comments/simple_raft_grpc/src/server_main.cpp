#include "simple_raft/server_application.h"

int main(int argc, char** argv) {
  // main 仅保留 C++ 程序入口；实际业务逻辑在 server_application.cpp。
  return simple_raft::ServerApplication::Run(argc, argv);
}
