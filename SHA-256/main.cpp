#include "sha256.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr
            << "usage: sha256_demo <file_path>\n";

        return 1;
    }

    try
    {
        const std::string filePath = argv[1];

        const std::string checksum =
            Sha256::calculateFile(filePath);

        std::cout
            << "file:   " << filePath << '\n'
            << "sha256: " << checksum << '\n';
    }
    catch (const std::exception &error)
    {
        std::cerr
            << "error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}