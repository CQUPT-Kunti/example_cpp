# Simple Raft gRPC（C++17，中文注释版）

这是一个教学用途的最小 Raft 一致性层示例，使用 Google gRPC + Protocol Buffers 实现节点间 RPC。它显式体现以下核心模块：

- Leader 选举：`ElectionManager`
- 日志持久化：`LogStore`
- 日志复制与一致性校验：`LogReplicator`
- 提交管理：`CommitManager`
- 节点追赶：`CatchUpManager`
- 节点恢复：`LogStore::Load` + `CommitManager::Recover`
- 网络通信：`PeerReplicator` + `RaftServiceImpl`

## 注释与文件组织

代码已补充中文说明，覆盖：类职责、每个参数的业务含义、返回值语义和关键执行流程。

| 位置 | 约定 |
|---|---|
| `include/simple_raft/*.h` | 所有结构体、类和函数的**声明**，以及中文接口说明。 |
| `src/*.cpp` | 所有函数的**实现和业务逻辑**，并注明关键 Raft 流程。 |
| `proto/raft.proto` | gRPC/Protobuf 接口定义及字段说明。 |
| `CMakeLists.txt` | 构建、proto 代码生成和链接关系。 |

`main` 函数仅保留标准 C++ 进程入口，转调 `ServerApplication::Run` 或 `ClientApplication::Run`；项目业务逻辑均不放在 `.h` 中。

## 核心链路

### 1. 选举

`RaftNode::StartElection()` 的流程：

1. 当前任期加一，持久化 `current_term`。
2. 本节点投票给自己，角色变为 `Candidate`。
3. 通过 gRPC 向所有 Peer 发送 `RequestVote`。
4. 发现任一更高任期，立即降级为 `Follower`。
5. 获得多数票后，升级为 `Leader`。

### 2. 日志复制与一致性校验

`RaftNode::ProposeCommand()` 只允许 Leader 接收写入：

1. 写命令先追加到 Leader 本地日志并持久化。
2. 调用 `AppendEntries` 复制给 Peer。
3. Follower 用 `prev_log_index + prev_log_term` 校验日志前缀。
4. 遇到相同 index、不同 term 的日志，Follower 截断冲突后缀，再追加 Leader 日志。
5. Leader 收到多数派成功响应后，推进 `commit_index`。

### 3. 提交与状态机

`CommitManager` 只会把 `commit_index` 以内的日志按序应用到状态机。示例状态机接受 `key=value` 命令，例如 `color=blue` 会写入内存 KV。

### 4. 节点恢复与追赶

节点启动时，`LogStore::Load()` 恢复 `current_term`、`voted_for`、日志和 `commit_index`；之后 `CommitManager::Recover()` 回放已提交日志到内存状态机。

`CatchUpManager` 可以从某个 `peer_match_index + 1` 开始取批量日志。本演示版本为便于阅读，每次复制均使用全量日志；正式系统应维护每个 Peer 的 `nextIndex` / `matchIndex` 并做增量发送与失败回退。

## 构建

依赖：CMake、C++17 编译器、Protocol Buffers、gRPC C++。依赖需要能够被 CMake `find_package(Protobuf CONFIG REQUIRED)` 与 `find_package(gRPC CONFIG REQUIRED)` 找到。

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

如果 gRPC / Protobuf 安装在自定义目录：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/your/grpc/install/prefix
cmake --build build -j
```

## 单节点演示

终端 1：

```bash
./build/raft_server --id node-1 --listen 127.0.0.1:50051 --data ./data/node-1
```

终端 2：

```bash
./build/raft_client 127.0.0.1:50051 elect
./build/raft_client 127.0.0.1:50051 propose color=blue
./build/raft_client 127.0.0.1:50051 status
```

单节点集群中，多数派为自身，因此选举后即可提交。

## 三节点演示

分别启动三个节点：

```bash
./build/raft_server --id node-1 --listen 127.0.0.1:50051 --data ./data/node-1 \
  --peer node-2=127.0.0.1:50052 --peer node-3=127.0.0.1:50053

./build/raft_server --id node-2 --listen 127.0.0.1:50052 --data ./data/node-2 \
  --peer node-1=127.0.0.1:50051 --peer node-3=127.0.0.1:50053

./build/raft_server --id node-3 --listen 127.0.0.1:50053 --data ./data/node-3 \
  --peer node-1=127.0.0.1:50051 --peer node-2=127.0.0.1:50052
```

然后手工触发选举并向成为 Leader 的节点提交命令：

```bash
./build/raft_client 127.0.0.1:50051 elect
./build/raft_client 127.0.0.1:50051 propose name=raft
```

## 设计边界

这是便于阅读的最小示例，而不是可直接上线的 Raft 实现。未实现自动选举超时、周期性心跳、成员变更、快照和日志压缩、TLS、WAL fsync 策略、每 Peer 的 `nextIndex/matchIndex` 回退、并行 RPC、线性一致读和生产级故障处理。
