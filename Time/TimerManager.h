#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>

#include "time.h"
class TimerManager
{
public:
    TimerManager();
    ~TimerManager();

    void add_timer(Timer *timer);
    void stop();

private:
    void run();
    struct Cmp
    {
        bool operator()(Timer *a, Timer *b) const
        {
            return a->expire_time > b->expire_time;
        }
    };
    std::priority_queue<Timer *, std::vector<Timer *>, Cmp> timer_queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stopped_flag = false;
    std::thread worker;
};