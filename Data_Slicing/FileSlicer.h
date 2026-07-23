#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "FileWriter.h"

// 切片信息
struct FileChunk
{
    std::uint64_t index;
    std::uint64_t offset;
    std::uint64_t size;
};

// 文件信息结构体
struct FileInfo
{
    std::string fileName;
    std::uint64_t fileSize;
    std::vector<FileChunk> chunks;
};

struct SlicerConfig
{
    std::uint64_t chunkSize;
    std::string outputDir;
};

class FileReader
{
private:
    std::ifstream input_;
    std::uint64_t fileSize;

public:
    FileReader() = default;
    ~FileReader() = default;

    bool open(const std::string &filePath);

    void close();

    bool seek(std::uint64_t offset);

    std::uint64_t read(char *buffer, std::uint64_t size);

    std::uint64_t size() const;

    bool is_open() const;
};

class FileSlicer
{
private:
    uint64_t chunk_size_;

    std::vector<FileChunk> chunks_;

public:
    bool slice(
        const std::string &filename,
        const std::string &output_dir);

    FileSlicer(const SlicerConfig &config);

    const std::vector<FileChunk> &getChunks() const;
};
