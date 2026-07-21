#include "FileSlicer.h"

#include <filesystem>
#include <iostream>

int main()
{
    const auto inputDir = std::filesystem::path("TestFile");
    const auto outputDir = std::filesystem::path("save");
    const std::uint64_t chunkSize = 20ULL * 1024 * 1024;

    std::filesystem::create_directories(outputDir);

    for (const auto &entry : std::filesystem::directory_iterator(inputDir))
    {
        if (!entry.is_regular_file())
        {
            continue;
        }

        FileSlicer slicer({chunkSize, outputDir.string()});
        if (!slicer.slice(entry.path().string(), outputDir.string()))
        {
            std::cerr << "slice failed: " << entry.path() << '\n';
            return 1;
        }

        std::cout << "file: " << entry.path().filename().string() << '\n';
        std::cout << "chunk size: " << chunkSize << " bytes\n";
        std::cout << "chunks: " << slicer.getChunks().size() << '\n';
    }

    return 0;
}
