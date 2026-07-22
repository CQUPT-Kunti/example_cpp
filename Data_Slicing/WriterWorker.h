#pragma once

#include "TaskQueue.h"

#include <atomic>
#include <memory>
#include <thread>

class WriterWorker
{
private:
    std::shared_ptr<TaskQueue> writerQueue_;
    std::shared_ptr<std::atomic_bool> ok_;
    std::thread thread_;

    void run();

public:
    WriterWorker(
        std::shared_ptr<TaskQueue> writerQueue,
        std::shared_ptr<std::atomic_bool> ok);

    void start();
    void join();
};
