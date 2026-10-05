#pragma once

#include "application/ports/IPackageInstaller.h"

class ApplyUpdate
{
public:
    explicit ApplyUpdate(const IPackageInstaller &installer);

    ApplyUpdateResult execute(const InstallRequest &request, const InstallProgress &progress) const;

private:
    const IPackageInstaller &m_installer;
};
