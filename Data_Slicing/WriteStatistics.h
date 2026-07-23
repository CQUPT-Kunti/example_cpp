#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>

class WriteStatistics
{
public:
    void addBytes(uint64_t size)
    {
        totalBytes_.fetch_add(size, std::memory_order_relaxed);
        cv_.notify_one();
    }

    uint64_t getBytes() const
    {
        return totalBytes_.load(std::memory_order_relaxed);
    }

    void reset()
    {
        totalBytes_.store(0, std::memory_order_relaxed);
    }

    bool waitFlush(
        uint64_t limit,
        std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return cv_.wait_for(lock, timeout, [this, limit]()
                            { return totalBytes_.load(std::memory_order_relaxed) >= limit; });
    }

private:
    std::atomic<uint64_t> totalBytes_{0};
    std::mutex mutex_;
    std::condition_variable cv_;
};
