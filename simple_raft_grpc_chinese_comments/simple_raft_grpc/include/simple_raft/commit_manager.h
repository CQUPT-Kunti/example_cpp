#pragma once

#include <cstdint>
#include <mutex>

#include "simple_raft/log_store.h"
#include "simple_raft/state_machine.h"

namespace simple_raft {

/**
 * @brief 维护 commit_index 与 last_applied，并将已提交日志顺序应用到状态机。
 */
class CommitManager {
 public:
  /**
   * @param log_store 存放提交位点和日志内容的持久化仓库。
   * @param state_machine 接收已提交日志的业务状态机。
   */
  CommitManager(LogStore& log_store, StateMachine& state_machine);

  /**
   * @brief 节点启动后的恢复入口。
   *
   * 流程：清空内存状态机 -> last_applied 重置为 0 -> 从日志 1 回放到磁盘 commit_index。
   */
  void Recover();

  /**
   * @brief 尝试推进提交位点并应用新增已提交日志。
   * @param target_commit_index Leader 或本地多数派确认的目标位点；会限制在 LastIndex 内。
   */
  void AdvanceCommit(std::uint64_t target_commit_index);

  /** @brief 返回当前已持久化的 commit_index。 */
  std::uint64_t CommitIndex() const;

  /** @brief 返回内存状态机已完成应用的最大日志序号。 */
  std::uint64_t LastApplied() const;

 private:
  /**
   * @brief 将区间 (last_applied_, target_commit_index] 的日志按序应用到状态机。
   * 调用方必须已经持有 mutex_。
   */
  void ApplyThrough(std::uint64_t target_commit_index);

  LogStore& log_store_;          ///< 提供日志和提交位点持久化。
  StateMachine& state_machine_;  ///< 执行已提交命令。
  std::uint64_t last_applied_;   ///< 已应用到状态机的最大序号，仅内存保存。
  mutable std::mutex mutex_;     ///< 保护回放和推进提交过程。
};

}  // namespace simple_raft
