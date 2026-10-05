#include "infrastructure/ExecutableDiscovery.h"

#include <QDir>
#include <QFileInfo>
#include <QString>

std::vector<std::string> ExecutableDiscovery::findExecutables(const std::string &directory, const std::string &selfFileName) const
{
    std::vector<std::string> found;
    const QDir dir(QString::fromStdString(directory));
#ifdef Q_OS_WIN
    const QFileInfoList entries = dir.entryInfoList({QStringLiteral("*.exe")}, QDir::Files);
    const Qt::CaseSensitivity sensitivity = Qt::CaseInsensitive;
#else
    const QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Executable);
    const Qt::CaseSensitivity sensitivity = Qt::CaseSensitive;
#endif
    const QString self = QString::fromStdString(selfFileName);
    for (const QFileInfo &info : entries) {
        if (!info.isFile() || info.fileName().compare(self, sensitivity) == 0)
            continue;
        found.push_back(info.absoluteFilePath().toStdString());
    }
    return found;
}
