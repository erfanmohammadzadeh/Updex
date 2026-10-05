#pragma once

#include "domain/Release.h"

#include <string>

struct ReleaseFetchResult
{
    bool success = false;
    Release release;
    std::string message;
};

class IReleaseRepository
{
public:
    virtual ~IReleaseRepository() = default;
    virtual ReleaseFetchResult fetchLatest(const std::string &repositoryUrl, const std::string &token) const = 0;
};
