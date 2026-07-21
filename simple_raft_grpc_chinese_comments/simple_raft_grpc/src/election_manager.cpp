#include "simple_raft/election_manager.h"

#include <utility>

namespace simple_raft {

ElectionManager::ElectionManager(std::string local_node_id, LogStore& log_store)
    : local_node_id_(std::move(local_node_id)),
      log_store_(log_store),
      role_(NodeRole::kFollower) {}

VoteRequestData ElectionManager::StartElection() {
  std::lock_guard<std::mutex> lock(mutex_);

  // 1. 每次新选举都进入更高任期，并将任期立即落盘。
  const std::uint64_t next_term = log_store_.CurrentTerm() + 1;
  log_store_.SetCurrentTerm(next_term);

  // 2. 候选人先投给自己；同一任期内后续不能再投给其他节点。
  log_store_.SetVotedFor(local_node_id_);
  role_ = NodeRole::kCandidate;
  leader_id_.clear();

  // 3. 携带末尾日志信息，让接收方拒绝日志明显落后的候选人。
  return VoteRequestData{next_term, local_node_id_, log_store_.LastIndex(), log_store_.LastTerm()};
}

VoteResponseData ElectionManager::HandleVoteRequest(const VoteRequestData& request) {
  std::lock_guard<std::mutex> lock(mutex_);

  const std::uint64_t local_term = log_store_.CurrentTerm();
  if (request.term < local_term) {
    // 候选人来自旧任期，不能获得投票；返回本地更高任期使其有机会降级。
    return VoteResponseData{local_term, false};
  }

  if (request.term > local_term) {
    // 看见更高任期时，本节点放弃旧任期的投票和角色状态。
    log_store_.SetCurrentTerm(request.term);
    log_store_.SetVotedFor("");
    role_ = NodeRole::kFollower;
    leader_id_.clear();
  }

  // 只有“本任期尚未投票/已投给同一候选人”且“候选人日志不落后”时才授予投票。
  const std::string voted_for = log_store_.VotedFor();
  const bool can_vote = voted_for.empty() || voted_for == request.candidate_id;
  const bool candidate_is_current = IsCandidateLogUpToDate(request);
  const bool granted = can_vote && candidate_is_current;
  if (granted) {
    log_store_.SetVotedFor(request.candidate_id);
  }
  return VoteResponseData{log_store_.CurrentTerm(), granted};
}

bool ElectionManager::ObserveLeader(std::uint64_t leader_term, const std::string& leader_id) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (leader_term < log_store_.CurrentTerm()) {
    // 不能接受陈旧 Leader 的日志，否则可能覆盖新任期数据。
    return false;
  }

  if (leader_term > log_store_.CurrentTerm()) {
    // 更高任期的 Leader 到来：更新任期并清空旧投票记录。
    log_store_.SetCurrentTerm(leader_term);
    log_store_.SetVotedFor("");
  }

  // 同任期 Candidate 也必须退回 Follower，承认已知 Leader。
  role_ = NodeRole::kFollower;
  leader_id_ = leader_id;
  return true;
}

bool ElectionManager::CompleteElection(std::uint64_t votes_received,
                                       std::uint64_t quorum_size) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 只有仍处于 Candidate 且票数达到多数派，才能成为该任期唯一 Leader。
  if (role_ != NodeRole::kCandidate || votes_received < quorum_size) {
    return false;
  }
  role_ = NodeRole::kLeader;
  leader_id_ = local_node_id_;
  return true;
}

void ElectionManager::StepDown(std::uint64_t new_term) {
  std::lock_guard<std::mutex> lock(mutex_);

  // 收到任意 RPC 响应中的高任期，都必须先更新本地任期，避免继续以旧 Leader 身份写入。
  if (new_term > log_store_.CurrentTerm()) {
    log_store_.SetCurrentTerm(new_term);
    log_store_.SetVotedFor("");
  }
  role_ = NodeRole::kFollower;
  leader_id_.clear();
}

NodeRole ElectionManager::Role() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return role_;
}

std::uint64_t ElectionManager::CurrentTerm() const {
  // CurrentTerm 自身由 LogStore 的 mutex_ 保护。
  return log_store_.CurrentTerm();
}

std::string ElectionManager::LeaderId() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return leader_id_;
}

bool ElectionManager::IsCandidateLogUpToDate(const VoteRequestData& request) const {
  const std::uint64_t local_last_term = log_store_.LastTerm();
  const std::uint64_t local_last_index = log_store_.LastIndex();

  // 末尾日志任期优先级更高；只有任期相同才比较长度。
  if (request.last_log_term != local_last_term) {
    return request.last_log_term > local_last_term;
  }
  return request.last_log_index >= local_last_index;
}

}  // namespace simple_raft
