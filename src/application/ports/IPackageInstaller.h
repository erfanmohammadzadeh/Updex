#pragma once

#include "application/UpdateResults.h"
#include "domain/InstalledApplication.h"
#include "domain/Release.h"

#include <functional>
#include <string>

struct InstallRequest
{
    Release release;
    Asset asset;
    InstalledApplication application;
    std::string token;
    std::string updaterExecutablePath;
};

using InstallProgress = std::function<void(int percent, const std::string &message)>;

class IPackageInstaller
{
public:
    virtual ~IPackageInstaller() = default;
    virtual ApplyUpdateResult install(const InstallRequest &request, const InstallProgress &progress) const = 0;
};
