#pragma once

#include <cstddef>
#include <vector>

class Buffer
{
public:
    explicit Buffer(std::size_t size);

    char *data();

    std::size_t size() const;

private:
    std::vector<char> data_;
};
