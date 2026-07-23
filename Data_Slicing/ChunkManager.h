#pragma once

#include "BufferPool.h"
#include "FileSlicer.h"
#include "ReaderWorker.h"
#include "WriterWorker.h"
#include "WriteStatistics.h"

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <vector>

class ChunkManager
{
private:
    SlicerConfig config_;
    std::vector<FileChunk> chunks_;
    std::shared_ptr<BufferPool> bufferPool_;
    std::vector<std::shared_ptr<ReaderWorker>> readerWorkers_;
    std::vector<std::shared_ptr<WriterWorker>> writerWorkers_;
    std::thread flush_work_;
    std::atomic_bool flush_running_{false};
    std::shared_ptr<WriteStatistics> statistics_;

    void flushLoop();

public:
    explicit ChunkManager(const SlicerConfig &config);
    bool slice(const std::string &filename, const std::string &output_dir);
    const std::vector<FileChunk> &getChunks() const;
};
