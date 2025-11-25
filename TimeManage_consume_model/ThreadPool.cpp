#include "ThreadPool.h"

ThreadPool::ThreadPool(int n)
{
    for (int i = 0; i < n; ++i)
    {
        workers.emplace_back(&ThreadPool::worker, this);
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        running = false;
    }

    cv.notify_all();
    for (auto &t : workers)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
}

void ThreadPool::worker()
{
    while (true)
    {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]
                    { return !running || !tasks.empty(); });

            if (!running && tasks.empty())
            {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}
