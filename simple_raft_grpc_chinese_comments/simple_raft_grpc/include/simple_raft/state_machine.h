#pragma once

#include <map>
#include <mutex>
#include <string>

#include "simple_raft/types.h"

namespace simple_raft {

/**
 * @brief 教学用内存 KV 状态机。
 *
 * 只识别 "key=value" 命令。Raft 保证已提交日志按序 Apply，因此各正常节点能够得到
 * 一致的 KV 结果。生产系统可替换为数据库、文件系统或业务领域状态机。
 */
class StateMachine {
 public:
  /**
   * @brief 将一条已提交日志应用到状态机。
   * @param entry 已经达到 commit_index 的日志；command 必须是 key=value 才会生效。
   */
  void Apply(const LogEntryData& entry);

  /**
   * @brief 读取一个键的当前值。
   * @param key 要查询的键。
   * @return 键不存在时返回空字符串；本示例不区分“空值”和“不存在”。
   */
  std::string Get(const std::string& key) const;

  /** @brief 清空内存状态；节点启动恢复时先清空再回放已提交日志。 */
  void Clear();

 private:
  std::map<std::string, std::string> values_;  ///< 有序 KV 数据。
  mutable std::mutex mutex_;                   ///< 保护状态机读写。
};

}  // namespace simple_raft
