#include "simple_raft/types.h"

namespace simple_raft {

std::string ToString(NodeRole role) {
  // 角色字符串用于 gRPC 返回值和命令行展示，避免调用方依赖枚举的底层整数值。
  switch (role) {
    case NodeRole::kFollower:
      return "follower";
    case NodeRole::kCandidate:
      return "candidate";
    case NodeRole::kLeader:
      return "leader";
  }

  // 理论上不会到达这里；保留兜底值，便于未来新增角色时发现遗漏。
  return "unknown";
}

}  // namespace simple_raft
