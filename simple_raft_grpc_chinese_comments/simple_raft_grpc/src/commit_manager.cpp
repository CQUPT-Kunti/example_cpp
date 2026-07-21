#include "simple_raft/commit_manager.h"

#include <algorithm>
#include <optional>

namespace simple_raft {

CommitManager::CommitManager(LogStore& log_store, StateMachine& state_machine)
    : log_store_(log_store), state_machine_(state_machine), last_applied_(0) {}

void CommitManager::Recover() {
  std::lock_guard<std::mutex> lock(mutex_);

  // 1. 内存状态机不可跨进程直接恢复，先清空其可能残留的内容。
  state_machine_.Clear();
  last_applied_ = 0;

  // 2. 只回放已经提交的日志，绝不能把未提交项暴露给业务状态机。
  ApplyThrough(log_store_.CommitIndex());
}

void CommitManager::AdvanceCommit(std::uint64_t target_commit_index) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Leader/Follower 都不能提交尚未存在于本地的日志。
  const std::uint64_t bounded_target = std::min(target_commit_index, log_store_.LastIndex());
  if (bounded_target <= log_store_.CommitIndex()) {
    // 提交位点只能单调向前；重复的提交通知直接忽略。
    return;
  }

  // 1. 先持久化 commit_index，确保崩溃恢复时知道哪些日志可以重放。
  log_store_.SetCommitIndex(bounded_target);

  // 2. 再按序应用新增区间到业务状态机。
  ApplyThrough(bounded_target);
}

std::uint64_t CommitManager::CommitIndex() const {
  return log_store_.CommitIndex();
}

std::uint64_t CommitManager::LastApplied() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return last_applied_;
}

void CommitManager::ApplyThrough(std::uint64_t target_commit_index) {
  // 由于 last_applied_ 是最后已应用位置，下一条必须从 last_applied_ + 1 开始。
  for (std::uint64_t index = last_applied_ + 1; index <= target_commit_index; ++index) {
    const std::optional<LogEntryData> entry = log_store_.EntryAt(index);
    if (!entry.has_value()) {
      // 日志缺失时宁可停止，不跳过 index；跳过会破坏状态机的确定性顺序。
      break;
    }

    state_machine_.Apply(*entry);
    last_applied_ = index;
  }
}

}  // namespace simple_raft
