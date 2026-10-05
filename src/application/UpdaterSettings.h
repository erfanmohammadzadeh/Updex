#pragma once

#include <string>

struct UpdaterSettings
{
    std::string repositoryUrl;
    std::string targetExecutable;
    std::string token;
    std::string assetContains;
    bool scheduleEnabled = false;
    int checkEvery = 1;
    std::string checkUnit = "day";
    long long lastCheckEpoch = 0;
};
