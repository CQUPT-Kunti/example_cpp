#include "WriterWorker.h"

#include "FileSlicer.h"

#include <memory>

WriterWorker::WriterWorker(
    std::shared_ptr<TaskQueue> writerQueue,
    std::shared_ptr<std::atomic_bool> ok)
    : writerQueue_(std::move(writerQueue)),
      ok_(std::move(ok))
{
}

void WriterWorker::start()
{
    thread_ = std::thread(&WriterWorker::run, this);
}

void WriterWorker::join()
{
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void WriterWorker::run()
{
    std::shared_ptr<ChunkTask> task;
    while (writerQueue_->pop(task))
    {
        if (!task->ok)
        {
            continue;
        }

        FileWriter writer;
        if (!writer.open(task->outputPath) ||
            !writer.write(task->buffer.data(), task->buffer.size()))
        {
            *ok_ = false;
        }
    }
}
