#pragma once

#include "BufferPool.h"
#include "TaskQueue.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>

class ReaderWorker
{
private:
    std::shared_ptr<TaskQueue> readerQueue_;
    std::shared_ptr<TaskQueue> writerQueue_;
    std::shared_ptr<BufferPool> bufferPool_;
    std::shared_ptr<std::atomic_bool> ok_;
    std::thread thread_;

    void run();

public:
    ReaderWorker(
        std::shared_ptr<TaskQueue> readerQueue,
        std::shared_ptr<TaskQueue> writerQueue,
        std::shared_ptr<BufferPool> bufferPool,
        std::shared_ptr<std::atomic_bool> ok);

    void start();
    void join();
};
