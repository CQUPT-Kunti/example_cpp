#pragma once

#include <cstdint>

#include "simple_raft/log_store.h"
#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 处理 Follower 侧的 AppendEntries 日志一致性校验和追加。
 *
 * 任期与 Leader 身份检查由 ElectionManager 完成；本类只聚焦日志序列本身。
 */
class LogReplicator {
 public:
  /** @param log_store 本地日志仓库。 */
  explicit LogReplicator(LogStore& log_store);

  /**
   * @brief 校验请求声明的“前序日志”是否存在且任期一致。
   * @param prev_log_index 批次第一条日志之前的序号；0 表示空前缀且总是合法。
   * @param prev_log_term 前序日志应该拥有的任期。
   * @return true 表示可安全接收后续 entries；false 表示需要 Leader 回退/追赶。
   */
  bool ValidatePreviousLog(std::uint64_t prev_log_index,
                           std::uint64_t prev_log_term) const;

  /**
   * @brief 接收 Leader 推送的一批日志。
   *
   * 流程：校验 prev_log -> 逐条检测同 index 日志 -> 任期冲突则截断本地后缀 ->
   * 追加缺失日志 -> 返回本地匹配进度。
   * @param request Leader 的 AppendEntries 内部请求。
   * @return 是否成功及成功匹配到的最后日志序号。
   */
  AppendResultData AppendFromLeader(const AppendEntriesRequestData& request);

 private:
  LogStore& log_store_;  ///< Follower 本地的可持久化日志。
};

}  // namespace simple_raft
