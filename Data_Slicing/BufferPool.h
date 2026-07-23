#pragma once

#include "Buffer.h"

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>

class BufferPool
{
public:
    BufferPool(
        std::size_t bufferCount,
        std::size_t bufferSize);

    std::shared_ptr<Buffer> acquire();

    void release(
        std::shared_ptr<Buffer> buffer);

private:
    std::queue<std::shared_ptr<Buffer>> buffers_;
    std::mutex mutex_;
    std::condition_variable cv_;
};
