#pragma once

#include "ChunkTask.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

class TaskQueue
{
private:
    std::queue<std::shared_ptr<ChunkTask>> tasks_;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;

public:
    void push(const std::shared_ptr<ChunkTask> &task);
    bool pop(std::shared_ptr<ChunkTask> &task);
    void close();
};
