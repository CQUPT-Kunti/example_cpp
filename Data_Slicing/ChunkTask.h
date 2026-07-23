#pragma once

#include "Buffer.h"
#include "FileSlicer.h"

#include <string>
#include <memory>

struct ChunkTask
{
    FileChunk chunk;
    std::string sourcePath;
    std::string outputPath;
    std::shared_ptr<Buffer> buffer;
    bool ok = true;
};
