#pragma once

#include <functional>
#include <chrono>
#include <atomic>

class TimerManager;

typedef std::function<void()> func;

class Timer
{
private:
    func callBackFunc;
    int timeOut_ms;
    bool repeat;
    std::chrono::steady_clock::time_point expire;
    std::atomic<bool> is_stopped{true};

public:
    Timer(int timeout_ms_, func func_, bool repeat_);

    void reset_expire();
    bool stopped() const { return is_stopped; }
    int get_timeout() const { return timeOut_ms; }
    bool is_repeat() const { return repeat; }
    void stop() { is_stopped = true; }
    void start() { is_stopped = false; }

    std::chrono::steady_clock::time_point get_expire() const { return expire; }
    void execute()
    {
        if (callBackFunc)
            callBackFunc();
    }

    bool operator>(const Timer &other) const
    {
        return expire > other.expire;
    }
};
