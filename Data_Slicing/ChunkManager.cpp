#include "ChunkManager.h"

#include "TaskQueue.h"

#include <algorithm>
#include <atomic>
#include <filesystem>

ChunkManager::ChunkManager(const SlicerConfig &config)
    : config_(config)
{
}

bool ChunkManager::slice(const std::string &filename, const std::string &output_dir)
{
    constexpr std::size_t kReaderCount = 4;
    constexpr std::size_t kWriterCount = 2;

    if (config_.chunkSize == 0)
    {
        return false;
    }

    chunks_.clear();
    readerWorkers_.clear();
    writerWorkers_.clear();
    std::filesystem::create_directories(output_dir);

    FileReader probe;
    if (!probe.open(filename))
    {
        return false;
    }
    const std::uint64_t fileSize = probe.size();
    probe.close();

    auto readerQueue = std::make_shared<TaskQueue>();
    auto writerQueue = std::make_shared<TaskQueue>();
    auto ok = std::make_shared<std::atomic_bool>(true);
    bufferPool_ = std::make_shared<BufferPool>(
        kReaderCount + kWriterCount,
        static_cast<std::size_t>(config_.chunkSize));

    for (std::size_t i = 0; i < kWriterCount; ++i)
    {
        auto worker = std::make_shared<WriterWorker>(writerQueue, bufferPool_, ok);
        worker->start();
        writerWorkers_.push_back(worker);
    }

    for (std::size_t i = 0; i < kReaderCount; ++i)
    {
        auto worker = std::make_shared<ReaderWorker>(readerQueue, writerQueue, bufferPool_, ok);
        worker->start();
        readerWorkers_.push_back(worker);
    }

    for (std::uint64_t offset = 0; offset < fileSize; offset += config_.chunkSize)
    {
        FileChunk chunk{
            offset / config_.chunkSize,
            offset,
            std::min(config_.chunkSize, fileSize - offset)};

        auto task = std::make_shared<ChunkTask>();
        task->chunk = chunk;
        task->sourcePath = filename;
        task->outputPath = output_dir + "/chunk_" + std::to_string(chunk.index);

        chunks_.push_back(chunk);
        readerQueue->push(task);
    }

    readerQueue->close();
    for (const auto &worker : readerWorkers_)
    {
        worker->join();
    }

    writerQueue->close();
    for (const auto &worker : writerWorkers_)
    {
        worker->join();
    }

    return ok->load();
}

const std::vector<FileChunk> &ChunkManager::getChunks() const
{
    return chunks_;
}
