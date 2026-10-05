#pragma once

#include "domain/Version.h"

#include <string>

struct InstalledApplication
{
    std::string executablePath;
    std::string directory;
    Version version;
    bool hasVersion = false;
};
