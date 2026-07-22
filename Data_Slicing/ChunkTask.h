#pragma once

#include "FileSlicer.h"

#include <string>
#include <vector>

struct ChunkTask
{
    FileChunk chunk;
    std::string sourcePath;
    std::string outputPath;
    std::vector<char> buffer;
    bool ok = true;
};
