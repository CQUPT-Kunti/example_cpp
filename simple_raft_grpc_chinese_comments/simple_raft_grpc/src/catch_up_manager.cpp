#include "simple_raft/catch_up_manager.h"

namespace simple_raft {

CatchUpManager::CatchUpManager(LogStore& log_store) : log_store_(log_store) {}

std::vector<LogEntryData> CatchUpManager::BuildCatchUpBatch(
    std::uint64_t peer_match_index, std::size_t max_entries) const {
  // 对端已确认到 peer_match_index，因此下一条需要发送 peer_match_index + 1。
  // 真实生产实现会为每个 Peer 维护 nextIndex/matchIndex；本示例保留此接口作为扩展点。
  return log_store_.EntriesFrom(peer_match_index + 1, max_entries);
}

}  // namespace simple_raft
