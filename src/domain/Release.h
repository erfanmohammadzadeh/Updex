#pragma once

#include "domain/Version.h"

#include <string>
#include <vector>

struct Asset
{
    std::string name;
    std::string downloadUrl;
    long long sizeBytes = 0;
};

struct Release
{
    std::string tag;
    std::string name;
    Version version;
    bool hasVersion = false;
    std::string notes;
    std::vector<Asset> assets;
};
