#pragma once

#include "application/ports/IExecutableDiscovery.h"

class ExecutableDiscovery final : public IExecutableDiscovery
{
public:
    std::vector<std::string> findExecutables(const std::string &directory, const std::string &selfFileName) const override;
};
