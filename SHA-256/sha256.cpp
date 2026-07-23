#include "sha256.h"

#include <openssl/evp.h>

#include <array>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

std::string sha256::calculateFile(const std::string &filePath)
{
    std::ifstream input(filePath, std::ios::binary);

    if (!input.is_open())
    {
        throw std::runtime_error(
            "cannot open file: " + filePath);
    }

    using ContextPtr =
        std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

    ContextPtr context(
        EVP_MD_CTX_new(),
        &EVP_MD_CTX_free);

    if (!context)
    {
        throw std::runtime_error(
            "failed to create SHA-256 context");
    }

    if (EVP_DigestInit_ex(
            context.get(),
            EVP_sha256(),
            nullptr) != 1)
    {
        throw std::runtime_error(
            "failed to initialize SHA-256");
    }

    constexpr std::size_t kBufferSize =
        1024 * 1024; // 1 MiB

    std::vector<unsigned char> buffer(kBufferSize);

    while (input)
    {
        input.read(
            reinterpret_cast<char *>(buffer.data()),
            static_cast<std::streamsize>(buffer.size()));

        const std::streamsize bytesRead =
            input.gcount();

        if (bytesRead > 0)
        {
            if (EVP_DigestUpdate(
                    context.get(),
                    buffer.data(),
                    static_cast<std::size_t>(bytesRead)) != 1)
            {
                throw std::runtime_error(
                    "failed to update SHA-256");
            }
        }
    }

    if (!input.eof())
    {
        throw std::runtime_error(
            "failed while reading file");
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0;

    if (EVP_DigestFinal_ex(
            context.get(),
            digest.data(),
            &digestLength) != 1)
    {
        throw std::runtime_error(
            "failed to finalize SHA-256");
    }

    std::ostringstream result;

    result << std::hex
           << std::setfill('0');

    for (unsigned int i = 0; i < digestLength; ++i)
    {
        result
            << std::setw(2)
            << static_cast<unsigned int>(digest[i]);
    }

    return result.str();
}