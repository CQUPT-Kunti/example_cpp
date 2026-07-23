#include "WriterWorker.h"

#include "FileSlicer.h"

#include <memory>

WriterWorker::WriterWorker(
    std::shared_ptr<TaskQueue> writerQueue,
    std::shared_ptr<BufferPool> bufferPool,
    std::shared_ptr<std::atomic_bool> ok)
    : writerQueue_(std::move(writerQueue)),
      bufferPool_(std::move(bufferPool)),
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
            bufferPool_->release(task->buffer);
            task->buffer.reset();
            continue;
        }

        FileWriter writer;
        if (!writer.open(task->outputPath) ||
            !writer.write(task->buffer->data(), task->chunk.size))
        {
            *ok_ = false;
        }

        writer.close();
        bufferPool_->release(task->buffer);
        task->buffer.reset();
    }
}
