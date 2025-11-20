#include "time.h"
#include "TimerManager.h"

Timer::Timer(std::function<void()> cb, int timeout_ms_, TimerManager *manager_)
{
    callback = cb;
    timeout_ms = timeout_ms_;
    manager = manager_;
}

void Timer::start()
{
    is_stopped = false;
    expire_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    manager->add_timer(this);
}

void Timer::restart(int ms)
{
    if (ms > 0)
        timeout_ms = ms;
    start();
}

void Timer::stop()
{
    is_stopped = true;
}