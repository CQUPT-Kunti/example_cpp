#include "Buffer.h"

Buffer::Buffer(std::size_t size)
    : data_(size)
{
}

char *Buffer::data()
{
    return data_.data();
}

std::size_t Buffer::size() const
{
    return data_.size();
}
