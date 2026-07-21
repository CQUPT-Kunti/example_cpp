cmake_minimum_required(VERSION 3.20)
project(simple_raft_grpc LANGUAGES CXX)

# 本项目使用 C++17；业务类的声明放 include/，实现放 src/。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# gRPC 和 Protocol Buffers 通过 CMake package 导入。
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# 构建时从 proto/raft.proto 自动生成 protobuf 与 gRPC C++ 桩代码。
set(PROTO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proto/raft.proto")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

add_custom_command(
  OUTPUT
    "${GENERATED_DIR}/raft.pb.cc"
    "${GENERATED_DIR}/raft.pb.h"
    "${GENERATED_DIR}/raft.grpc.pb.cc"
    "${GENERATED_DIR}/raft.grpc.pb.h"
  COMMAND $<TARGET_FILE:protobuf::protoc>
  ARGS
    --cpp_out="${GENERATED_DIR}"
    --grpc_out="${GENERATED_DIR}"
    -I "${CMAKE_CURRENT_SOURCE_DIR}/proto"
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    "${PROTO_FILE}"
  DEPENDS "${PROTO_FILE}"
  VERBATIM
)

# 生成代码单独形成库，供服务端和客户端共用。
add_library(raft_proto
  "${GENERATED_DIR}/raft.pb.cc"
  "${GENERATED_DIR}/raft.grpc.pb.cc"
)
target_include_directories(raft_proto PUBLIC "${GENERATED_DIR}")
target_link_libraries(raft_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)

# 领域逻辑与 gRPC 服务适配器。每个 src/*.cpp 对应 include/simple_raft/*.h 的声明。
add_library(raft_core
  src/types.cpp
  src/log_store.cpp
  src/election_manager.cpp
  src/log_replicator.cpp
  src/state_machine.cpp
  src/commit_manager.cpp
  src/catch_up_manager.cpp
  src/peer_replicator.cpp
  src/raft_node.cpp
  src/raft_service_impl.cpp
)
target_include_directories(raft_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_link_libraries(raft_core PUBLIC raft_proto)

# raft_server：启动节点和 gRPC Server。
add_executable(raft_server
  src/server_main.cpp
  src/server_application.cpp
)
target_link_libraries(raft_server PRIVATE raft_core)

# raft_client：演示 status/elect/propose 三个 gRPC 调用。
add_executable(raft_client
  src/client_main.cpp
  src/client_application.cpp
)
target_link_libraries(raft_client PRIVATE raft_proto)
