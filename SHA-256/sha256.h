#pragma once

#include <string>

class sha256
{
private:
    /* data */
public:
    static std::string calculateFile(const std::string &filePath);
};
