# simple_raft_grpc 代码阅读路线图

本文档用于指导你阅读 `simple_raft_grpc` 示例项目。目标不是一次理解全部文件，而是先沿着一个具体流程把关键代码串起来，再分别阅读选举、日志复制、提交和恢复模块。

---

## 1. 先记住项目的主入口

服务端程序启动后，最先经过下面这条链路：

```text
src/server_main.cpp
  └── main()
        └── ServerApplication::Run()
              ├── 创建 NodeConfig
              ├── 创建 RaftNode
              ├── RaftNode::Initialize()
              ├── 创建 RaftServiceImpl
              └── gRPC Server::Wait()
```

因此，**建议的第一个阅读起点**是：

1. `src/server_main.cpp` 的 `main()`
2. `src/server_application.cpp` 的 `ServerApplication::Run()`
3. `src/raft_node.cpp` 的 `RaftNode::Initialize()`

这里先弄明白三件事：

- 一个节点由哪些配置组成：节点 ID、监听地址、数据目录、Peer 列表。
- 节点启动时如何从磁盘恢复：`LogStore::Load()`。
- 恢复后为什么还要回放：`CommitManager::Recover()` 会把已提交日志重新应用到状态机。

> 阅读原则：`.h` 文件先看类职责、成员变量和函数声明；真正的执行过程再去同名 `.cpp` 文件看。

---

## 2. 项目文件地图

| 文件 | 从哪个函数开始看 | 作用 |
|---|---|---|
| `proto/raft.proto` | `service RaftService` | 定义 gRPC 的请求与响应结构，以及 RPC 接口。 |
| `src/server_main.cpp` | `main()` | 服务端进程入口。 |
| `src/server_application.cpp` | `ServerApplication::Run()` | 解析启动参数，创建 Raft 节点和 gRPC 服务。 |
| `src/raft_service_impl.cpp` | `RaftServiceImpl::Propose()` | gRPC 请求进入 Raft 业务层的适配入口。 |
| `src/raft_node.cpp` | `RaftNode::ProposeCommand()` | 核心编排层：选举、写入、复制、提交、接收复制请求。 |
| `src/log_store.cpp` | `LogStore::Load()` | 任期、投票记录、日志、提交位点的持久化与恢复。 |
| `src/log_replicator.cpp` | `LogReplicator::AppendFromLeader()` | Follower 侧日志一致性校验、冲突截断和追加。 |
| `src/commit_manager.cpp` | `CommitManager::AdvanceCommit()` | 推进 `commit_index` 并按顺序应用日志。 |
| `src/state_machine.cpp` | `StateMachine::Apply()` | 把已提交的 `key=value` 命令真正写入内存 KV 状态。 |
| `src/election_manager.cpp` | `ElectionManager::StartElection()` | 候选人发起选举、投票判断、成为 Leader、降级。 |
| `src/peer_replicator.cpp` | `PeerReplicator::RequestVotes()` | 通过 gRPC 向其他节点发送投票和复制请求。 |
| `src/catch_up_manager.cpp` | `CatchUpManager::BuildCatchUpBatch()` | 按对端进度组装需要补发的日志。 |
| `src/client_application.cpp` | `ClientApplication::Run()` | 演示客户端，发起 `status`、`elect`、`propose` 请求。 |

---

## 3. 推荐阅读顺序：先看一次“写入并提交”

不要先看所有类。先只看客户端提交一条命令时发生什么。

假设你执行：

```bash
./build/raft_client 127.0.0.1:50051 propose color=blue
```

请严格按照下面的函数顺序阅读：

```text
1. src/client_application.cpp
   ClientApplication::Run()

2. src/raft_service_impl.cpp
   RaftServiceImpl::Propose()

3. src/raft_node.cpp
   RaftNode::ProposeCommand()

4. src/log_store.cpp
   LogStore::Append()
   LogStore::SaveUnlocked()

5. src/raft_node.cpp
   RaftNode::BuildFullReplicationRequest()

6. src/peer_replicator.cpp
   PeerReplicator::Replicate()
   PeerReplicator::SendAppendToPeer()

7. 对端节点：src/raft_service_impl.cpp
   RaftServiceImpl::AppendEntries()

8. 对端节点：src/raft_node.cpp
   RaftNode::HandleAppendEntries()

9. 对端节点：src/log_replicator.cpp
   LogReplicator::AppendFromLeader()

10. Leader：src/commit_manager.cpp
    CommitManager::AdvanceCommit()
    CommitManager::ApplyThrough()

11. Leader：src/state_machine.cpp
    StateMachine::Apply()
```

### 这条链路中，每一步要看什么

#### 第 1 步：`ClientApplication::Run()`

文件：`src/client_application.cpp`

查看 `command == "propose"` 这段逻辑。

你需要理解：客户端只是把字符串 `color=blue` 放入 protobuf 的 `CommandRequest`，然后调用 gRPC 的 `Propose` RPC。

#### 第 2 步：`RaftServiceImpl::Propose()`

文件：`src/raft_service_impl.cpp`

这个函数不实现 Raft 算法。它只做适配：

```text
protobuf 请求 -> 内部业务调用 -> protobuf 响应
```

重点看这一句：

```cpp
raft_node_.ProposeCommand(request->command())
```

这表示真正的 Raft 写入逻辑从 `RaftNode::ProposeCommand()` 开始。

#### 第 3 步：`RaftNode::ProposeCommand()`

文件：`src/raft_node.cpp`

这是**客户端写入的核心入口**。按函数中的注释顺序阅读：

1. 判断当前节点是否为 Leader。
2. 创建一条带有 `index`、`term` 和 `command` 的日志。
3. 调用 `LogStore::Append()` 将日志写入本地磁盘。
4. 调用 `PeerReplicator::Replicate()` 向其他节点复制日志。
5. 判断成功副本数是否达到多数派。
6. 达到多数派后调用 `CommitManager::AdvanceCommit()`。
7. 最后通知 Follower 推进 `leader_commit`。

必须记住：**日志已经写到 Leader 本地，不代表命令已经提交。只有复制到多数派后才提交。**

#### 第 4 步：`LogStore::Append()` 与 `LogStore::SaveUnlocked()`

文件：`src/log_store.cpp`

先看 `LogStore::Append()`：

- 检查日志 `index` 是否连续。
- 将日志加入 `entries_`。
- 调用 `SaveUnlocked()` 保存到 `raft.log`。

再看 `SaveUnlocked()`：

- 先写 `raft.log.tmp` 临时文件。
- 写入 `META`：任期、提交位置、已投票节点。
- 写入每一条 `LOG`。
- 再替换正式的 `raft.log`。

这就是本项目中“日志持久化”的实现位置。

#### 第 5 步：`PeerReplicator::Replicate()` 与 `SendAppendToPeer()`

文件：`src/peer_replicator.cpp`

这两个函数负责 Leader 到 Follower 的 gRPC 调用。

- `Replicate()`：遍历所有 Peer，统计 `success=true` 的数量。
- `SendAppendToPeer()`：将内部 `AppendEntriesRequestData` 转成 protobuf，并调用对端 `AppendEntries` RPC。

这里的实现采用同步串行 RPC，便于阅读。生产系统通常会并发发送，并为每个 Follower 单独维护复制进度。

#### 第 6 步：Follower 的 `RaftNode::HandleAppendEntries()`

文件：`src/raft_node.cpp`

这是 Follower 接收 Leader 日志的核心入口。按顺序看：

1. `ElectionManager::ObserveLeader()`：检查 Leader 任期是否过旧；高任期或同任期 Leader 会让本节点退回 Follower。
2. `LogReplicator::AppendFromLeader()`：校验和追加日志。
3. `CommitManager::AdvanceCommit()`：根据 `leader_commit` 推进本地提交位置。

#### 第 7 步：`LogReplicator::AppendFromLeader()`

文件：`src/log_replicator.cpp`

这是**日志一致性校验和冲突修复的核心函数**。

顺序如下：

1. 调用 `ValidatePreviousLog()` 检查 `prev_log_index` 与 `prev_log_term`。
2. 若前序日志不匹配，返回 `success=false`，不修改本地日志。
3. 如果同一 `index` 的本地日志与 Leader 日志 `term` 不同，调用 `LogStore::TruncateSuffix()` 删除冲突后缀。
4. 对本地不存在的日志调用 `LogStore::Append()`。

要特别理解下面的 Raft 规则：

> 同一个日志 index 上 term 不同，意味着旧 Leader 的未提交后缀与当前 Leader 冲突；Follower 必须删除冲突位置及之后的日志，再接受当前 Leader 的日志。

#### 第 8 步：`CommitManager::AdvanceCommit()` 与 `StateMachine::Apply()`

文件：`src/commit_manager.cpp`、`src/state_machine.cpp`

`AdvanceCommit()` 做两件事：

1. 调用 `LogStore::SetCommitIndex()` 持久化提交位置。
2. 调用 `ApplyThrough()`，按日志序号依次应用尚未执行的已提交日志。

`StateMachine::Apply()` 则将 `key=value` 解析为内存 KV：

```text
color=blue
  ↓
values_["color"] = "blue"
```

---

## 4. 第二条阅读线：Leader 选举

在理解写入流程后，再看选举。

手动触发选举的命令：

```bash
./build/raft_client 127.0.0.1:50051 elect
```

阅读函数顺序：

```text
1. src/client_application.cpp
   ClientApplication::Run()

2. src/raft_service_impl.cpp
   RaftServiceImpl::StartElection()

3. src/raft_node.cpp
   RaftNode::StartElection()

4. src/election_manager.cpp
   ElectionManager::StartElection()

5. src/peer_replicator.cpp
   PeerReplicator::RequestVotes()
   PeerReplicator::RequestVoteFromPeer()

6. 对端节点：src/raft_service_impl.cpp
   RaftServiceImpl::RequestVote()

7. 对端节点：src/raft_node.cpp
   RaftNode::HandleVoteRequest()

8. 对端节点：src/election_manager.cpp
   ElectionManager::HandleVoteRequest()

9. 回到发起节点：src/election_manager.cpp
   ElectionManager::CompleteElection()
```

### 选举最需要关注的函数

#### `ElectionManager::StartElection()`

文件：`src/election_manager.cpp`

按顺序理解：

1. `current_term + 1`。
2. 将新任期持久化。
3. 给自己投票，并将 `voted_for` 持久化。
4. 将角色切换为 `Candidate`。
5. 携带本节点最后一条日志的 `index` 和 `term`，生成 `VoteRequestData`。

#### `ElectionManager::HandleVoteRequest()`

这是投票节点的判断逻辑。重点看两个条件：

```text
can_vote && candidate_is_current
```

含义是：

- 本任期还没投票，或者已经投给这个候选人。
- 候选人的日志不能比本节点旧。

其中日志新旧判断在：

```cpp
ElectionManager::IsCandidateLogUpToDate()
```

规则是：先比较最后日志的 `term`；`term` 相同才比较 `index`。

#### `ElectionManager::CompleteElection()`

只有满足下面两个条件，Candidate 才会成为 Leader：

```text
当前角色仍然是 Candidate
且 votes_received >= quorum_size
```

多数派大小由：

```cpp
RaftNode::QuorumSize()
```

计算：

```text
集群节点数 = peers 数量 + 自己
多数派 = 集群节点数 / 2 + 1
```

---

## 5. 第三条阅读线：节点重启后的数据恢复

这个流程用于理解“日志保存”和“数据恢复”。

阅读函数顺序：

```text
src/server_main.cpp
  └── main()
        └── ServerApplication::Run()
              └── RaftNode::Initialize()
                    ├── LogStore::Load()
                    └── CommitManager::Recover()
                          └── CommitManager::ApplyThrough()
                                └── StateMachine::Apply()
```

### 重点函数

#### `LogStore::Load()`

文件：`src/log_store.cpp`

从 `raft.log` 中恢复：

- `current_term`
- `voted_for`
- `commit_index`
- 全部 `LogEntryData`

同时会校正一个异常情况：如果持久化的 `commit_index` 超过最后一条日志，就将它回退到最后日志位置。

#### `CommitManager::Recover()`

文件：`src/commit_manager.cpp`

重启后内存状态机是空的，所以要：

1. `StateMachine::Clear()` 清理内存状态。
2. 将 `last_applied_` 重置为 0。
3. 从日志 1 开始，重新应用到 `commit_index`。

关键原则：**只回放已经提交的日志，不能回放未提交日志。**

---

## 6. 第四条阅读线：节点追赶

本项目提供了节点追赶的模块边界，但为了让流程清晰，实际发送时采用“全量日志复制”的简化方式。

建议阅读：

```text
src/raft_node.cpp
  RaftNode::BuildFullReplicationRequest()

src/catch_up_manager.cpp
  CatchUpManager::BuildCatchUpBatch()

src/log_store.cpp
  LogStore::EntriesFrom()
```

逻辑是：

```text
对端已匹配到 peer_match_index
  ↓
从 peer_match_index + 1 开始读取日志
  ↓
最多读取 max_entries 条
  ↓
作为下一批 AppendEntries 发送给对端
```

当前演示实现中：

```cpp
RaftNode::BuildFullReplicationRequest()
```

传入 `peer_match_index = 0`，表示从第一条日志开始发送全部日志。这样可以直观看到追赶和冲突修复，但不是生产级增量复制实现。

---

## 7. 如果你只想先读最核心的 6 个文件

按下面顺序阅读即可：

```text
1. proto/raft.proto
2. src/server_application.cpp
3. src/raft_service_impl.cpp
4. src/raft_node.cpp
5. src/log_replicator.cpp
6. src/commit_manager.cpp
```

读完这 6 个文件，你应该能回答：

- 客户端写入从哪里进入系统？
- 为什么 Follower 不能接收客户端写？
- 日志如何落盘？
- Follower 如何判断日志是否冲突？
- 为什么必须复制到多数派后才能提交？
- `commit_index` 和 `last_applied` 有什么区别？

---

## 8. 建议使用断点阅读

若使用 CLion、VS Code + GDB 或 gdb，优先在下面函数打断点：

```text
simple_raft::RaftNode::ProposeCommand
simple_raft::LogStore::Append
simple_raft::PeerReplicator::SendAppendToPeer
simple_raft::RaftNode::HandleAppendEntries
simple_raft::LogReplicator::AppendFromLeader
simple_raft::CommitManager::AdvanceCommit
simple_raft::StateMachine::Apply

simple_raft::RaftNode::StartElection
simple_raft::ElectionManager::StartElection
simple_raft::ElectionManager::HandleVoteRequest
simple_raft::ElectionManager::CompleteElection
```

观察变量时，优先看：

```text
current_term
role_
leader_id_
entries_
last_log_index
commit_index
last_applied_
prev_log_index
prev_log_term
match_index
```

---

## 9. 读完后应该建立的整体模型

可以把这个项目理解为下面 6 个模块的协作：

```text
ElectionManager
  管理任期、投票、Candidate/Leader/Follower 角色

LogStore
  持久化任期、投票、日志、commit_index

LogReplicator
  校验前序日志、删除冲突后缀、追加 Leader 日志

PeerReplicator
  使用 gRPC 发送 RequestVote 和 AppendEntries

CommitManager
  多数派确认后推进 commit_index，顺序应用日志

StateMachine
  执行业务命令；本示例中就是 key=value 写入 KV
```

`RaftNode` 是总编排者：它不重复实现每个模块内部细节，而是把这些模块串为一次完整的 Raft 流程。

---

## 10. 本示例的简化点

阅读时需要知道以下能力是刻意简化的：

- 没有基于随机选举超时的自动选举，使用 `StartElection` 手工触发。
- 没有固定周期心跳。
- 没有维护生产级的每 Peer `nextIndex` / `matchIndex`。
- 写入复制时发送全量日志，而不是增量批次。
- 没有快照压缩、成员变更、线性一致性读、持久化 fsync 策略和完整崩溃原子性处理。

这些简化不影响你理解本项目中展示的 Raft 主干：**选举、日志持久化、日志复制、一致性校验、提交、追赶与恢复。**
