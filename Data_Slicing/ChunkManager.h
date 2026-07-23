#pragma once

#include "BufferPool.h"
#include "FileSlicer.h"
#include "ReaderWorker.h"
#include "WriterWorker.h"

#include <memory>
#include <string>
#include <vector>

class ChunkManager
{
private:
    SlicerConfig config_;
    std::vector<FileChunk> chunks_;
    std::shared_ptr<BufferPool> bufferPool_;
    std::vector<std::shared_ptr<ReaderWorker>> readerWorkers_;
    std::vector<std::shared_ptr<WriterWorker>> writerWorkers_;

public:
    explicit ChunkManager(const SlicerConfig &config);

    bool slice(const std::string &filename, const std::string &output_dir);

    const std::vector<FileChunk> &getChunks() const;
};
