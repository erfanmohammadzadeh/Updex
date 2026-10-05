#include "infrastructure/ExecutableDiscovery.h"

#include <QDir>
#include <QFileInfo>
#include <QString>

std::vector<std::string> ExecutableDiscovery::findExecutables(const std::string &directory, const std::string &selfFileName) const
{
    std::vector<std::string> found;
    const QFileInfoList entries = QDir(QString::fromStdString(directory)).entryInfoList({QStringLiteral("*.exe")}, QDir::Files);
    const QString self = QString::fromStdString(selfFileName);
    for (const QFileInfo &info : entries) {
        if (info.fileName().compare(self, Qt::CaseInsensitive) == 0)
            continue;
        found.push_back(info.absoluteFilePath().toStdString());
    }
    return found;
}
