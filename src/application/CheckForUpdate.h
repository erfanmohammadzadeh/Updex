#pragma once

#include "application/UpdateResults.h"
#include "application/UpdaterSettings.h"
#include "application/ports/IApplicationProbe.h"
#include "application/ports/IReleaseRepository.h"

class CheckForUpdate
{
public:
    CheckForUpdate(const IReleaseRepository &releases, const IApplicationProbe &probe);

    UpdateCheckResult execute(const UpdaterSettings &settings, const std::string &resolvedExecutable) const;

private:
    const IReleaseRepository &m_releases;
    const IApplicationProbe &m_probe;
};
