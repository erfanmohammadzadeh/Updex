#pragma once

#include "domain/Release.h"

#include <string>

struct UpdateCheckResult
{
    bool success = false;
    bool updateAvailable = false;
    bool canUpdate = false;
    bool hasLocalVersion = false;
    Version localVersion;
    Release latest;
    bool hasLatest = false;
    Asset selectedAsset;
    std::string resolvedExecutable;
    std::string message;
};

struct ApplyUpdateResult
{
    bool success = false;
    std::string message;
    int filesReplaced = 0;
    std::string backupDirectory;
};
