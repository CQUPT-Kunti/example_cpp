#include "WriterWorker.h"

#include <memory>

WriterWorker::WriterWorker(
    std::shared_ptr<TaskQueue> writerQueue,
    std::shared_ptr<BufferPool> bufferPool,
    std::shared_ptr<WriteStatistics> statistics,
    std::shared_ptr<std::atomic_bool> ok)
    : writerQueue_(std::move(writerQueue)),
      bufferPool_(std::move(bufferPool)),
      statistics_(std::move(statistics)),
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

        if (!writer_.open(task->outputPath) ||
            !writer_.write(task->buffer->data(), task->chunk.size))
        {
            *ok_ = false;
        }
        statistics_->addBytes(task->chunk.size);
        writer_.close();
        bufferPool_->release(task->buffer);
        task->buffer.reset();
    }
}

void WriterWorker::flush()
{
    writer_.flush();
}
