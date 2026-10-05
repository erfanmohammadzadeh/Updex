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
#ifdef UPDEX_OS_WINDOWS
    return toLowerCopy(path);
#else
    return path;
#endif
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

    const bool zip = endsWithCi(request.asset.name, ".zip");
    const bool singleFile = endsWithCi(request.asset.name, ".exe")
        || endsWithCi(request.asset.name, ".appimage")
        || endsWithCi(request.asset.name, ".bin")
        || request.asset.name.find('.') == std::string::npos;
    if (!zip && !singleFile)
        return {false, "The release asset must be a .zip of the program files, or a single program file.", 0, {}};

    return m_installer.install(request, progress);
}
