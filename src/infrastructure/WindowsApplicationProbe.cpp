#include "infrastructure/WindowsApplicationProbe.h"

#include <QFile>
#include <QFileInfo>
#include <QString>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winver.h>

#include <vector>

namespace
{
bool readFileVersion(const QString &path, Version *version)
{
    const std::wstring wide = path.toStdWString();
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(wide.c_str(), &handle);
    if (size == 0)
        return false;

    std::vector<unsigned char> data(size);
    if (!GetFileVersionInfoW(wide.c_str(), 0, size, data.data()))
        return false;

    VS_FIXEDFILEINFO *info = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<void **>(&info), &length))
        return false;
    if (!info || length < sizeof(VS_FIXEDFILEINFO))
        return false;

    version->major = HIWORD(info->dwFileVersionMS);
    version->minor = LOWORD(info->dwFileVersionMS);
    version->patch = HIWORD(info->dwFileVersionLS);
    version->build = LOWORD(info->dwFileVersionLS);
    version->componentCount = 4;
    return version->major || version->minor || version->patch || version->build;
}

bool readVersionFile(const QString &directory, Version *version)
{
    QFile file(directory + QStringLiteral("/version.txt"));
    if (!file.open(QIODevice::ReadOnly))
        return false;
    QString text = QString::fromUtf8(file.readAll()).trimmed();
    if (text.startsWith(QChar(0xFEFF)))
        text.remove(0, 1);
    const auto parsed = Version::parse(text.toStdString());
    if (!parsed)
        return false;
    *version = *parsed;
    return true;
}
}

InstalledApplication WindowsApplicationProbe::inspect(const std::string &executablePath) const
{
    InstalledApplication app;
    const QFileInfo info(QString::fromStdString(executablePath));
    if (!info.exists() || !info.isFile())
        return app;

    app.executablePath = info.absoluteFilePath().toStdString();
    app.directory = info.absolutePath().toStdString();
    if (readFileVersion(info.absoluteFilePath(), &app.version) || readVersionFile(info.absolutePath(), &app.version))
        app.hasVersion = true;
    return app;
}
