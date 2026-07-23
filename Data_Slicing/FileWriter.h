#pragma once

#include <cstdint>
#include <fstream>
#include <mutex>
#include <string>

class FileWriter
{
private:
    std::ofstream output_;
    mutable std::mutex mutex_;

public:
    FileWriter() = default;
    ~FileWriter() = default;

    bool open(const std::string &filePath);

    void close();

    bool write(const char *buffer, std::uint64_t size);

    void flush();

    bool is_open() const;
};
