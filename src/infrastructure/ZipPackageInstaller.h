#pragma once

#include "application/ports/IPackageInstaller.h"

class ZipPackageInstaller final : public IPackageInstaller
{
public:
    ApplyUpdateResult install(const InstallRequest &request, const InstallProgress &progress) const override;
};
