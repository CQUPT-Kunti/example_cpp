#include "BufferPool.h"

BufferPool::BufferPool(
    std::size_t bufferCount,
    std::size_t bufferSize)
{
    for (std::size_t i = 0; i < bufferCount; ++i)
    {
        buffers_.push(std::make_shared<Buffer>(bufferSize));
    }
}

std::shared_ptr<Buffer> BufferPool::acquire()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !buffers_.empty(); });

    auto buffer = buffers_.front();
    buffers_.pop();
    return buffer;
}

void BufferPool::release(std::shared_ptr<Buffer> buffer)
{
    if (!buffer)
    {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        buffers_.push(std::move(buffer));
    }
    cv_.notify_one();
}
