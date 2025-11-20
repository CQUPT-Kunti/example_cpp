#include "TimerManager.h"
TimerManager::TimerManager()
{
    worker = std::thread(&TimerManager::run, this);
}

TimerManager::~TimerManager()
{
    stop();
    if (worker.joinable())
        worker.join();
}

void TimerManager::add_timer(Timer *time)
{
    std::lock_guard<std::mutex> lock(mtx);
    timer_queue.push(time);
    time->manager = this;
    cv.notify_one();
}

void TimerManager::stop()
{
    {
        std::unique_lock<std::mutex> lock(mtx);
        stopped_flag = true;
    }
    cv.notify_one();
}

void TimerManager::run()
{
    while (true)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (stopped_flag)
            break;

        if (timer_queue.empty())
        {
            cv.wait(lock);
            continue;
        }

        Timer *t = timer_queue.top();
        auto now = std::chrono::steady_clock::now();

        if (t->is_stopped)
        {
            timer_queue.pop();
            continue;
        }

        if (t->expire_time > now)
        {
            cv.wait_until(lock, t->expire_time);
            continue;
        }

        timer_queue.pop();

        lock.unlock();
        t->callback();
    }
}