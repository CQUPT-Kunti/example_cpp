#include "FileWriter.h"

bool FileWriter::open(const std::string &filePath)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_.is_open())
    {
        output_.close();
    }
    output_.open(filePath, std::ios::binary);
    return output_.is_open();
}

void FileWriter::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_.is_open())
    {
        output_.close();
    }
}

bool FileWriter::write(const char *buffer, std::uint64_t size)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!output_.is_open())
    {
        return false;
    }

    output_.write(buffer, size);
    return !output_.fail();
}

void FileWriter::flush()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (output_.is_open())
    {
        output_.flush();
    }
}

bool FileWriter::is_open() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return output_.is_open();
}
