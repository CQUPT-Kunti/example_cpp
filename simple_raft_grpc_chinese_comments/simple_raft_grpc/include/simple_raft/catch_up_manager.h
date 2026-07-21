#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "simple_raft/log_store.h"
#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 为落后副本构造待补齐的日志批次。
 *
 * 简化实现中，RaftNode 每次从 index=1 构造全量复制请求以便演示；该类也支持按
 * peer_match_index 做增量提取，便于后续扩展 nextIndex/matchIndex 优化。
 */
class CatchUpManager {
 public:
  /** @param log_store Leader 本地的完整日志来源。 */
  explicit CatchUpManager(LogStore& log_store);

  /**
   * @brief 生成一个副本从已匹配位置之后开始需要追赶的日志。
   * @param peer_match_index 对端已确认匹配的最大日志序号。
   * @param max_entries 单批最多携带的日志条数。
   * @return [peer_match_index + 1, ...] 的连续日志副本。
   */
  std::vector<LogEntryData> BuildCatchUpBatch(std::uint64_t peer_match_index,
                                              std::size_t max_entries) const;

 private:
  LogStore& log_store_;  ///< Leader 日志来源。
};

}  // namespace simple_raft
