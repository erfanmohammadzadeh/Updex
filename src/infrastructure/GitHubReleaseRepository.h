#pragma once

#include "application/ports/IReleaseRepository.h"

class GitHubReleaseRepository final : public IReleaseRepository
{
public:
    ReleaseFetchResult fetchLatest(const std::string &repositoryUrl, const std::string &token) const override;
};
