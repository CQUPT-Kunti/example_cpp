#include "TaskQueue.h"

void TaskQueue::push(const std::shared_ptr<ChunkTask> &task)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_)
        {
            return;
        }
        tasks_.push(task);
    }
    cv_.notify_one();
}

bool TaskQueue::pop(std::shared_ptr<ChunkTask> &task)
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return closed_ || !tasks_.empty(); });

    if (tasks_.empty())
    {
        return false;
    }

    task = tasks_.front();
    tasks_.pop();
    return true;
}

void TaskQueue::close()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    cv_.notify_all();
}
