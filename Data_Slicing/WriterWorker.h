#pragma once

#include "BufferPool.h"
#include "FileWriter.h"
#include "TaskQueue.h"
#include "WriteStatistics.h"

#include <atomic>
#include <memory>
#include <thread>

class WriterWorker
{
private:
    std::shared_ptr<TaskQueue> writerQueue_;
    std::shared_ptr<BufferPool> bufferPool_;
    std::shared_ptr<WriteStatistics> statistics_;
    std::shared_ptr<std::atomic_bool> ok_;
    FileWriter writer_;
    std::thread thread_;

    void run();

public:
    WriterWorker(
        std::shared_ptr<TaskQueue> writerQueue,
        std::shared_ptr<BufferPool> bufferPool,
        std::shared_ptr<WriteStatistics> statistics,
        std::shared_ptr<std::atomic_bool> ok);

    void start();
    void join();
    void flush();
};
