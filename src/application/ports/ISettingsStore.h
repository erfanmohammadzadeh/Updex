#pragma once

#include "application/UpdaterSettings.h"

#include <string>

class ISettingsStore
{
public:
    virtual ~ISettingsStore() = default;
    virtual bool load(UpdaterSettings &settings, std::string &error) const = 0;
    virtual bool save(const UpdaterSettings &settings) = 0;
    virtual std::string filePath() const = 0;
};
