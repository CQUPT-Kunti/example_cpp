#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace simple_raft {

/**
 * @brief 节点在 Raft 中的角色。
 *
 * kFollower：被动接收 Leader 的日志或投票请求；
 * kCandidate：正在发起选举、收集投票；
 * kLeader：当前唯一接受客户端写入并向其他节点复制日志的节点。
 */
enum class NodeRole {
  kFollower,
  kCandidate,
  kLeader
};

/** @brief 集群中一个远端 Raft 节点的连接信息。 */
struct PeerEndpoint {
  std::string id;       ///< 对端节点标识，例如 "node-2"。
  std::string address;  ///< 对端 gRPC 地址，例如 "127.0.0.1:50052"。
};

/** @brief 创建 RaftNode 时传入的静态节点配置。 */
struct NodeConfig {
  std::string node_id;                 ///< 本节点唯一 ID。
  std::string listen_address;          ///< 本节点 gRPC 服务监听地址。
  std::string data_directory;          ///< 日志、任期、投票信息的落盘目录。
  std::vector<PeerEndpoint> peers;     ///< 除本节点以外的全部成员。
};

/**
 * @brief 一条必须按序复制并持久化的 Raft 日志。
 *
 * 约定 command 使用 "key=value" 格式；示例状态机会将其应用为 KV 写入。
 */
struct LogEntryData {
  std::uint64_t index;  ///< 全局递增日志序号，从 1 开始；0 代表“没有前序日志”。
  std::uint64_t term;   ///< 创建该日志时的 Leader 任期。
  std::string command;  ///< 要提交给状态机的业务命令。
};

/** @brief RequestVote RPC 的内部请求模型。 */
struct VoteRequestData {
  std::uint64_t term;            ///< 候选人的任期。
  std::string candidate_id;      ///< 请求投票的候选节点 ID。
  std::uint64_t last_log_index;  ///< 候选人最后一条日志的序号。
  std::uint64_t last_log_term;   ///< 候选人最后一条日志的任期。
};

/** @brief RequestVote RPC 的内部响应模型。 */
struct VoteResponseData {
  std::uint64_t term;  ///< 接收方当前任期；候选人据此判断自己是否需要降级。
  bool vote_granted;   ///< true 表示接收方投票给该候选人。
};

/**
 * @brief AppendEntries RPC 的内部请求模型。
 *
 * 同一接口承载两类消息：带 entries 时复制日志；entries 为空时传播最新提交位点，
 * 在完整 Raft 中也会用于心跳。
 */
struct AppendEntriesRequestData {
  std::uint64_t term;              ///< Leader 当前任期。
  std::string leader_id;           ///< 发起请求的 Leader 节点 ID。
  std::uint64_t prev_log_index;    ///< 本批日志之前一条日志的序号；0 代表空日志前缀。
  std::uint64_t prev_log_term;     ///< prev_log_index 对应日志的任期。
  std::uint64_t leader_commit;     ///< Leader 已确认提交的最大日志序号。
  std::vector<LogEntryData> entries;  ///< 需要追加到 Follower 的日志批次。
};

/** @brief AppendEntries RPC 的内部响应模型。 */
struct AppendResultData {
  std::uint64_t term;         ///< 接收方当前任期。
  bool success;               ///< 是否通过前序日志校验并完成本批追加。
  std::uint64_t match_index;  ///< 接收方成功匹配到的最大日志序号。
};

/** @brief 一次手动触发选举后的结果。 */
struct ElectionResultData {
  std::uint64_t term;           ///< 选举结束后的本地任期。
  bool became_leader;           ///< 是否取得多数票并成为 Leader。
  std::uint64_t votes_received; ///< 包含自身选票在内的已获得票数。
  NodeRole role;                ///< 选举结束后的本地角色。
};

/** @brief 客户端提交命令后的结果。 */
struct CommandResultData {
  bool accepted;                ///< 是否已达到多数派并提交成功。
  std::string message;          ///< 面向调用方的结果说明。
  std::uint64_t log_index;      ///< 该命令对应的日志序号；未写入时为 0。
  std::uint64_t commit_index;   ///< 返回时本节点的已提交位点。
};

/** @brief 查询节点当前运行状态的结果。 */
struct StatusData {
  std::string node_id;          ///< 本节点 ID。
  NodeRole role;                ///< 当前角色。
  std::uint64_t current_term;   ///< 当前任期。
  std::string leader_id;        ///< 已知 Leader ID；未知时为空。
  std::uint64_t last_log_index; ///< 本地最后一条日志序号。
  std::uint64_t commit_index;   ///< 已提交但未必都已查询到的最大序号。
  std::uint64_t last_applied;   ///< 已应用到状态机的最大序号。
};

/**
 * @brief 将枚举角色转换为便于 RPC、日志输出和 CLI 显示的字符串。
 * @param role 要转换的节点角色。
 * @return "follower"、"candidate"、"leader" 或 "unknown"。
 */
std::string ToString(NodeRole role);

}  // namespace simple_raft
