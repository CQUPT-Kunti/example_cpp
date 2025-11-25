#include "TimerManager.h"

TimerManager::TimerManager(ThreadPool &pool) : thread_pool(pool)
{
    manager_thread = std::thread(&TimerManager::run, this);
}

TimerManager::~TimerManager()
{
    stop();
}

void TimerManager::stop()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }
    cv.notify_all();

    if (manager_thread.joinable())
    {
        manager_thread.join();
    }
}

std::shared_ptr<Timer> TimerManager::add_timer(int timeout_ms, func callback, bool repeat)
{
    auto timer = std::make_shared<Timer>(timeout_ms, callback, repeat);
    timer->start();

    {
        std::lock_guard<std::mutex> lock(mtx);
        timer_queue.push(timer);
    }

    cv.notify_one();
    return timer;
}

void TimerManager::remove_timer(std::shared_ptr<Timer> timer)
{
    if (timer)
    {
        timer->stop();
    }
}

void TimerManager::run()
{
    while (running)
    {
        std::unique_lock<std::mutex> lock(mtx);

        if (timer_queue.empty())
        {
            cv.wait(lock, [this]
                    { return !running || !timer_queue.empty(); });
            if (!running)
                break;
        }

        auto now = std::chrono::steady_clock::now();
        auto timer = timer_queue.top();

        if (timer->get_expire() <= now)
        {
            timer_queue.pop();

            if (!timer->stopped())
            {
                thread_pool.submit([timer]()
                                   { timer->execute(); });

                if (timer->is_repeat() && !timer->stopped())
                {
                    timer->reset_expire();
                    timer_queue.push(timer);
                }
            }
        }
        else
        {
            auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(
                timer->get_expire() - now);
            cv.wait_for(lock, wait_time, [this]
                        { return !running; });
        }
    }
}
