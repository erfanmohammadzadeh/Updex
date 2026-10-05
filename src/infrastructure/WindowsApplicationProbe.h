#pragma once

#include "application/ports/IApplicationProbe.h"

class WindowsApplicationProbe final : public IApplicationProbe
{
public:
    InstalledApplication inspect(const std::string &executablePath) const override;
};
