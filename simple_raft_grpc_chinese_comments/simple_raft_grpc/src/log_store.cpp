#include "simple_raft/log_store.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace simple_raft {

LogStore::LogStore(std::string data_directory)
    : data_directory_(std::move(data_directory)), current_term_(0), commit_index_(0) {}

void LogStore::Load() {
  std::lock_guard<std::mutex> lock(mutex_);

  // 1. 确保存储目录存在；首次启动时该目录通常尚未创建。
  std::filesystem::create_directories(data_directory_);

  // 2. 清空内存中的旧状态，避免同一实例重复 Load 时混入历史数据。
  entries_.clear();
  current_term_ = 0;
  voted_for_.clear();
  commit_index_ = 0;

  // 3. 没有日志文件代表一个全新节点，保留初始空状态即可。
  std::ifstream input(FilePath());
  if (!input.is_open()) {
    return;
  }

  // 文件格式非常简单：一行 META 保存元数据；多行 LOG 保存日志。
  // std::quoted 允许 command/voted_for 中安全包含空格和引号。
  std::string record_type;
  while (input >> record_type) {
    if (record_type == "META") {
      input >> current_term_ >> commit_index_ >> std::quoted(voted_for_);
    } else if (record_type == "LOG") {
      LogEntryData entry{};
      input >> entry.index >> entry.term >> std::quoted(entry.command);
      entries_.push_back(std::move(entry));
    } else {
      // 日志格式不认识时直接报错，避免以不可信数据继续提供一致性服务。
      throw std::runtime_error("Unsupported record in raft log: " + record_type);
    }
  }

  // 4. 恢复时保证内存日志按 index 有序，以便二分查找和连续追加。
  std::sort(entries_.begin(), entries_.end(), [](const LogEntryData& left,
                                                 const LogEntryData& right) {
    return left.index < right.index;
  });

  // 5. 异常关机或手工篡改可能导致 commit_index 超过最后日志；这里做保守修正。
  const std::uint64_t last_index = entries_.empty() ? 0 : entries_.back().index;
  if (commit_index_ > last_index) {
    commit_index_ = last_index;
    SaveUnlocked();
  }
}

bool LogStore::Append(const LogEntryData& entry) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Raft 日志必须连续；不能跳号追加，否则后续 prev_log 校验会失去意义。
  const std::uint64_t expected_index = entries_.empty() ? 1 : entries_.back().index + 1;
  if (entry.index != expected_index) {
    return false;
  }

  // 先更新内存，再整体写盘。写盘失败时本示例会返回 false；生产系统还需要更严谨回滚。
  entries_.push_back(entry);
  return SaveUnlocked();
}

bool LogStore::TruncateSuffix(std::uint64_t first_index_to_remove) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Follower 发现同一 index 的任期不同，说明从该位置起是旧 Leader 留下的冲突后缀。
  const auto first_to_remove = std::lower_bound(
      entries_.begin(), entries_.end(), first_index_to_remove,
      [](const LogEntryData& entry, std::uint64_t index) { return entry.index < index; });
  entries_.erase(first_to_remove, entries_.end());

  // 截断后提交位点不得指向已删除日志。正常 Raft 不会删除已提交项，此处是额外保护。
  const std::uint64_t last_index = entries_.empty() ? 0 : entries_.back().index;
  if (commit_index_ > last_index) {
    commit_index_ = last_index;
  }
  return SaveUnlocked();
}

std::optional<LogEntryData> LogStore::EntryAt(std::uint64_t index) const {
  std::lock_guard<std::mutex> lock(mutex_);

  // entries_ 有序，使用二分查找而非线性扫描。
  const auto entry = std::lower_bound(
      entries_.begin(), entries_.end(), index,
      [](const LogEntryData& value, std::uint64_t target) { return value.index < target; });
  if (entry == entries_.end() || entry->index != index) {
    return std::nullopt;
  }
  return *entry;
}

std::vector<LogEntryData> LogStore::EntriesFrom(std::uint64_t first_index,
                                                std::size_t max_entries) const {
  std::lock_guard<std::mutex> lock(mutex_);

  std::vector<LogEntryData> result;
  const auto start = std::lower_bound(
      entries_.begin(), entries_.end(), first_index,
      [](const LogEntryData& value, std::uint64_t target) { return value.index < target; });

  // max_entries 是对网络包大小的保护；追赶时可按批循环调用。
  for (auto iterator = start; iterator != entries_.end() && result.size() < max_entries;
       ++iterator) {
    result.push_back(*iterator);
  }
  return result;
}

std::vector<LogEntryData> LogStore::AllEntries() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_;
}

std::uint64_t LogStore::LastIndex() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.empty() ? 0 : entries_.back().index;
}

std::uint64_t LogStore::LastTerm() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return entries_.empty() ? 0 : entries_.back().term;
}

std::uint64_t LogStore::CurrentTerm() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return current_term_;
}

std::string LogStore::VotedFor() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return voted_for_;
}

std::uint64_t LogStore::CommitIndex() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return commit_index_;
}

void LogStore::SetCurrentTerm(std::uint64_t term) {
  std::lock_guard<std::mutex> lock(mutex_);

  // current_term 是 Raft 必须持久化的安全状态：节点重启后不能回到旧任期。
  current_term_ = term;
  SaveUnlocked();
}

void LogStore::SetVotedFor(const std::string& node_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 同一任期只能投给一个候选人，故 voted_for 也必须立即持久化。
  voted_for_ = node_id;
  SaveUnlocked();
}

void LogStore::SetCommitIndex(std::uint64_t commit_index) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 提交位点不能超出本地已有日志；Follower 可能先收到 leader_commit，再收到后续日志。
  commit_index_ = std::min(commit_index, entries_.empty() ? 0 : entries_.back().index);
  SaveUnlocked();
}

bool LogStore::SaveUnlocked() const {
  // 调用此函数时 mutex_ 已被外层方法持有，不能在这里再次加锁，防止互斥锁重入。
  std::filesystem::create_directories(data_directory_);
  const std::string temporary_path = FilePath() + ".tmp";

  // 1. 先写临时文件，避免直接覆写正式文件时进程崩溃导致完整日志丢失。
  std::ofstream output(temporary_path, std::ios::trunc);
  if (!output.is_open()) {
    return false;
  }

  output << "META " << current_term_ << ' ' << commit_index_ << ' ' << std::quoted(voted_for_)
         << '\n';
  for (const LogEntryData& entry : entries_) {
    output << "LOG " << entry.index << ' ' << entry.term << ' ' << std::quoted(entry.command)
           << '\n';
  }
  output.flush();
  if (!output.good()) {
    return false;
  }
  output.close();

  // 2. 临时文件写完整后，再替换正式文件。
  // 注意：这里是跨平台教学实现；生产环境还应控制 fsync 和原子 rename 语义。
  std::error_code error;
  std::filesystem::remove(FilePath(), error);
  std::filesystem::rename(temporary_path, FilePath(), error);
  return !error;
}

std::string LogStore::FilePath() const {
  return data_directory_ + "/raft.log";
}

}  // namespace simple_raft
