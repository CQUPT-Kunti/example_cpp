#include "ReaderWorker.h"

#include "FileSlicer.h"

#include <utility>

ReaderWorker::ReaderWorker(
    std::shared_ptr<TaskQueue> readerQueue,
    std::shared_ptr<TaskQueue> writerQueue,
    std::shared_ptr<std::atomic_bool> ok)
    : readerQueue_(std::move(readerQueue)),
      writerQueue_(std::move(writerQueue)),
      ok_(std::move(ok))
{
}

void ReaderWorker::start()
{
    thread_ = std::thread(&ReaderWorker::run, this);
}

void ReaderWorker::join()
{
    if (thread_.joinable())
    {
        thread_.join();
    }
}

void ReaderWorker::run()
{
    std::shared_ptr<ChunkTask> task;
    while (readerQueue_->pop(task))
    {
        FileReader reader;
        if (!reader.open(task->sourcePath))
        {
            task->ok = false;
            *ok_ = false;
            writerQueue_->push(task);
            continue;
        }

        task->buffer.resize(task->chunk.size);
        if (!reader.seek(task->chunk.offset) ||
            reader.read(task->buffer.data(), task->chunk.size) != task->chunk.size)
        {
            task->ok = false;
            *ok_ = false;
        }
        reader.close();
        writerQueue_->push(task);
    }
}
