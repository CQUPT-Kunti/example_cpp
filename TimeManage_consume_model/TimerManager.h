#pragma once

#include "Timer.h"
#include "ThreadPool.h"
#include <memory>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class TimerManager
{
private:
    struct TimerCompare
    {
        bool operator()(const std::shared_ptr<Timer> &a, const std::shared_ptr<Timer> &b) const
        {
            return *a > *b;
        }
    };

    std::priority_queue<std::shared_ptr<Timer>,
                        std::vector<std::shared_ptr<Timer>>,
                        TimerCompare>
        timer_queue;

    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> running{true};
    std::thread manager_thread;
    ThreadPool &thread_pool;

    void run();

public:
    explicit TimerManager(ThreadPool &pool);
    ~TimerManager();

    std::shared_ptr<Timer> add_timer(int timeout_ms, func callback, bool repeat = false);
    void remove_timer(std::shared_ptr<Timer> timer);
    void stop();
};
