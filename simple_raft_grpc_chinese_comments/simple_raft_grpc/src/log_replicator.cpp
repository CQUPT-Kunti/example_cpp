#include "simple_raft/log_replicator.h"

#include <algorithm>

namespace simple_raft {

LogReplicator::LogReplicator(LogStore& log_store) : log_store_(log_store) {}

bool LogReplicator::ValidatePreviousLog(std::uint64_t prev_log_index,
                                        std::uint64_t prev_log_term) const {
  // prev_log_index=0 是约定的虚拟哨兵，表示从日志第一条开始复制。
  if (prev_log_index == 0) {
    return true;
  }

  // Raft 的一致性检查：索引和任期必须同时匹配，才能保证此前缀相同。
  const std::optional<LogEntryData> entry = log_store_.EntryAt(prev_log_index);
  return entry.has_value() && entry->term == prev_log_term;
}

AppendResultData LogReplicator::AppendFromLeader(const AppendEntriesRequestData& request) {
  // 1. 先验证前序日志。不一致时不修改本地日志，Leader 应根据失败响应重试/回退。
  if (!ValidatePreviousLog(request.prev_log_index, request.prev_log_term)) {
    return AppendResultData{log_store_.CurrentTerm(), false, log_store_.LastIndex()};
  }

  // 2. 逐条处理 Leader 发送的连续日志。
  for (const LogEntryData& incoming : request.entries) {
    const std::optional<LogEntryData> local = log_store_.EntryAt(incoming.index);

    // 3. 同一 index 存在不同 term，代表本地保留了旧 Leader 的冲突后缀。
    //    删除冲突项及其之后全部日志，随后由当前 Leader 覆盖。
    if (local.has_value() && local->term != incoming.term) {
      if (!log_store_.TruncateSuffix(incoming.index)) {
        return AppendResultData{log_store_.CurrentTerm(), false, log_store_.LastIndex()};
      }
    }

    // 4. 如果该 index 在本地不存在，才真正追加；完全相同的日志无需重复写盘。
    const std::optional<LogEntryData> after_truncate = log_store_.EntryAt(incoming.index);
    if (!after_truncate.has_value() && !log_store_.Append(incoming)) {
      return AppendResultData{log_store_.CurrentTerm(), false, log_store_.LastIndex()};
    }
  }

  // 5. 日志追加本身成功。leader_commit 的推进由 RaftNode 在该函数返回后处理。
  return AppendResultData{log_store_.CurrentTerm(), true, log_store_.LastIndex()};
}

}  // namespace simple_raft
