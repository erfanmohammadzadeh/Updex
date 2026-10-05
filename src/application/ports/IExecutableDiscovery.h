#pragma once

#include <string>
#include <vector>

class IExecutableDiscovery
{
public:
    virtual ~IExecutableDiscovery() = default;
    virtual std::vector<std::string> findExecutables(const std::string &directory, const std::string &selfFileName) const = 0;
};
