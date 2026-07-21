#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief Raft 持久化日志仓库。
 *
 * 本示例将 current_term、voted_for、commit_index 与所有 LogEntryData 一起保存到
 * data_directory/raft.log。每次修改后采用“写临时文件，再重命名”的简单策略。
 * 这是教学级实现；生产系统通常使用 WAL、fsync 策略、校验和和快照。
 */
class LogStore {
 public:
  /**
   * @param data_directory 日志文件所在目录；文件名固定为 raft.log。
   */
  explicit LogStore(std::string data_directory);

  /**
   * @brief 从磁盘恢复任期、投票、提交位点与日志。
   *
   * 流程：创建目录 -> 清空内存缓存 -> 读取 raft.log -> 按 index 排序 ->
   * 校正不能超过最后一条日志的 commit_index。
   */
  void Load();

  /**
   * @brief 追加一条连续的新日志并立即持久化。
   * @param entry 待追加日志；其 index 必须恰好等于 LastIndex() + 1。
   * @return true 表示写入内存和文件均成功；false 表示序号不连续或文件写失败。
   */
  bool Append(const LogEntryData& entry);

  /**
   * @brief 删除从指定序号开始的整段未匹配日志。
   * @param first_index_to_remove 需要删除的第一条日志序号；该值大于最后序号时无副作用。
   * @return true 表示删除后的状态已持久化。
   */
  bool TruncateSuffix(std::uint64_t first_index_to_remove);

  /** @brief 查询指定序号的日志。不存在时返回 std::nullopt。 */
  std::optional<LogEntryData> EntryAt(std::uint64_t index) const;

  /**
   * @brief 提取从 first_index 开始、数量最多为 max_entries 的连续日志批次。
   * @param first_index 批次起始日志序号。
   * @param max_entries 单次最多返回多少条，防止追赶 RPC 过大。
   */
  std::vector<LogEntryData> EntriesFrom(std::uint64_t first_index,
                                        std::size_t max_entries) const;

  /** @brief 返回当前日志副本；主要用于诊断或演示。 */
  std::vector<LogEntryData> AllEntries() const;

  /** @brief 返回最后一条日志的序号；空日志返回 0。 */
  std::uint64_t LastIndex() const;

  /** @brief 返回最后一条日志的任期；空日志返回 0。 */
  std::uint64_t LastTerm() const;

  /** @brief 返回当前持久化任期。 */
  std::uint64_t CurrentTerm() const;

  /** @brief 返回本任期已经投票给的节点 ID；空字符串代表尚未投票。 */
  std::string VotedFor() const;

  /** @brief 返回已确认提交的最大日志序号。 */
  std::uint64_t CommitIndex() const;

  /** @brief 设置并持久化当前任期。 */
  void SetCurrentTerm(std::uint64_t term);

  /** @brief 设置并持久化本任期的投票对象；传入空字符串可清空。 */
  void SetVotedFor(const std::string& node_id);

  /**
   * @brief 设置并持久化已提交位点。
   * @param commit_index 目标提交位点；内部会限制在最后日志序号以内。
   */
  void SetCommitIndex(std::uint64_t commit_index);

 private:
  /**
   * @brief 将当前内存状态写入临时文件，并替换正式日志文件。
   *
   * 调用者必须已经持有 mutex_；因此函数名带 Unlocked 后缀。
   */
  bool SaveUnlocked() const;

  /** @brief 拼接正式持久化文件的绝对/相对路径。 */
  std::string FilePath() const;

  std::string data_directory_;          ///< 日志根目录。
  std::vector<LogEntryData> entries_;   ///< 按 index 递增排列的内存日志。
  std::uint64_t current_term_;          ///< 需要持久化的当前任期。
  std::string voted_for_;               ///< 需要持久化的当前任期投票对象。
  std::uint64_t commit_index_;          ///< 需要持久化的提交位点。
  mutable std::mutex mutex_;            ///< 保护以上所有持久化状态。
};

}  // namespace simple_raft
