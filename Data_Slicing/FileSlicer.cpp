#include <algorithm>
#include <iostream>

#include "FileSlicer.h"

bool FileReader::open(const std::string &filePath)
{
    input_.open(filePath, std::ios::binary);
    if (!input_.is_open())
    {
        return false;
    }

    // 获取文件大小
    input_.seekg(0, std::ios::end);
    fileSize = input_.tellg();
    input_.seekg(0, std::ios::beg);
    return true;
}

void FileReader::close()
{
    if (input_.is_open())
    {
        input_.close();
    }
}

bool FileReader::seek(std::uint64_t offset)
{
    if (!input_.is_open())
    {
        return false;
    }

    input_.seekg(offset, std::ios::beg);
    return !input_.fail();
}

std::uint64_t FileReader::read(char *buffer, std::uint64_t size)
{
    if (!input_.is_open())
    {
        return 0;
    }

    input_.read(buffer, size);
    return input_.gcount();
}

bool FileReader::is_open() const
{
    return input_.is_open();
}

std::uint64_t FileReader::size() const
{
    return fileSize;
}

FileSlicer::FileSlicer(const SlicerConfig &config)
    : chunk_size_(config.chunkSize)
{
}

bool FileSlicer::slice(const std::string &filename, const std::string &output_dir)
{
    chunks_.clear();

    FileReader reader;
    if (!reader.open(filename))
    {
        return false;
    }

    std::uint64_t fileSize = reader.size();

    for (std::uint64_t offset = 0; offset < fileSize; offset += chunk_size_)
    {
        std::uint64_t currentChunkSize = std::min(chunk_size_, fileSize - offset);
        std::vector<char> buffer(currentChunkSize);
        reader.seek(offset);
        std::uint64_t bytesRead = reader.read(buffer.data(), currentChunkSize);
        if (bytesRead != currentChunkSize)
        {
            reader.close();
            return false;
        }

        std::string chunkFilename = output_dir + "/chunk_" + std::to_string(offset / chunk_size_);
        FileWriter writer;
        if (!writer.open(chunkFilename))
        {
            reader.close();
            return false;
        }

        if (!writer.write(buffer.data(), currentChunkSize))
        {
            reader.close();
            writer.close();
            return false;
        }
        chunks_.push_back({offset / chunk_size_, offset, currentChunkSize});
        writer.close();
    }
    reader.close();
    return true;
}

const std::vector<FileChunk> &FileSlicer::getChunks() const
{
    return chunks_;
}
