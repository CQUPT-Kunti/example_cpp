#pragma once

#include <chrono>
#include <functional>
#include <atomic>

class TimerManager;

class Timer
{
public:
    Timer(std::function<void()> cb, int timeout_ms_, TimerManager *manager_);

    void start();
    void stop();
    void restart(int new_timeout_ms = -1);

    bool stopped() const { return is_stopped; }

private:
    friend class TimerManager;
    std::function<void()> callback;
    int timeout_ms;
    std::chrono::steady_clock::time_point expire_time;
    std::atomic<bool> is_stopped{true};
    TimerManager *manager = nullptr;
};