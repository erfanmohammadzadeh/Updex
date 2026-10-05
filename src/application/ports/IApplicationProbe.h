#pragma once

#include "domain/InstalledApplication.h"

#include <string>

class IApplicationProbe
{
public:
    virtual ~IApplicationProbe() = default;
    virtual InstalledApplication inspect(const std::string &executablePath) const = 0;
};
