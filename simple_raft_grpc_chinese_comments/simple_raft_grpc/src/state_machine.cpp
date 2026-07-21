#include "simple_raft/state_machine.h"

namespace simple_raft {

void StateMachine::Apply(const LogEntryData& entry) {
  // 示例命令约定为 key=value。格式不合法的命令不修改状态机。
  const std::size_t separator = entry.command.find('=');
  if (separator == std::string::npos) {
    return;
  }

  // 分隔符左侧为 key，右侧为 value；value 允许继续包含 '='。
  const std::string key = entry.command.substr(0, separator);
  const std::string value = entry.command.substr(separator + 1);

  // Raft 保证已提交日志的 Apply 顺序；互斥锁保证本地并发读写的容器安全。
  std::lock_guard<std::mutex> lock(mutex_);
  values_[key] = value;
}

std::string StateMachine::Get(const std::string& key) const {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto value = values_.find(key);
  return value == values_.end() ? "" : value->second;
}

void StateMachine::Clear() {
  // 崩溃恢复时，先删除旧内存状态，然后从已提交日志重新回放。
  std::lock_guard<std::mutex> lock(mutex_);
  values_.clear();
}

}  // namespace simple_raft
