#include "application/ApplyUpdate.h"

#include "application/Text.h"

namespace
{
std::string normalizedPath(std::string path)
{
    for (char &c : path) {
        if (c == '\\')
            c = '/';
    }
    return toLowerCopy(path);
}
}

ApplyUpdate::ApplyUpdate(const IPackageInstaller &installer)
    : m_installer(installer)
{
}

ApplyUpdateResult ApplyUpdate::execute(const InstallRequest &request, const InstallProgress &progress) const
{
    if (request.application.executablePath.empty())
        return {false, "Program path is empty.", 0, {}};

    if (!request.updaterExecutablePath.empty()
        && normalizedPath(request.application.executablePath) == normalizedPath(request.updaterExecutablePath)) {
        return {false, "Choose the program to update. Updex does not replace itself.", 0, {}};
    }

    if (request.asset.downloadUrl.empty())
        return {false, "The release asset has no download URL.", 0, {}};

    if (!endsWithCi(request.asset.name, ".zip") && !endsWithCi(request.asset.name, ".exe"))
        return {false, "The release asset must be a .zip of the program files, or a .exe.", 0, {}};

    return m_installer.install(request, progress);
}
