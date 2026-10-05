#include "infrastructure/JsonSettingsStore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

JsonSettingsStore::JsonSettingsStore(const QString &directory)
    : m_filePath(QDir(directory).filePath(QStringLiteral("updex.json")))
{
}

bool JsonSettingsStore::load(UpdaterSettings &settings, std::string &error) const
{
    settings = {};
    error.clear();

    QFile file(m_filePath);
    if (!file.exists())
        return true;
    if (!file.open(QIODevice::ReadOnly)) {
        error = "Could not read " + m_filePath.toStdString();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        error = "updex.json is not valid JSON.";
        return false;
    }

    const QJsonObject obj = doc.object();
    const QString repository = obj.value(QStringLiteral("repository")).toString().trimmed();
    settings.repositoryUrl = (repository.isEmpty() ? obj.value(QStringLiteral("repo")).toString() : repository).trimmed().toStdString();
    settings.targetExecutable = obj.value(QStringLiteral("targetExecutable")).toString().trimmed().toStdString();
    settings.token = obj.value(QStringLiteral("token")).toString().trimmed().toStdString();
    settings.assetContains = obj.value(QStringLiteral("assetContains")).toString().trimmed().toStdString();
    settings.scheduleEnabled = obj.value(QStringLiteral("scheduleEnabled")).toBool(false);
    if (obj.contains(QStringLiteral("checkEvery"))) {
        settings.checkEvery = obj.value(QStringLiteral("checkEvery")).toInt(1);
        const QString unit = obj.value(QStringLiteral("checkUnit")).toString().trimmed().toLower();
        settings.checkUnit = unit == QStringLiteral("week") ? "week" : "day";
    } else if (obj.contains(QStringLiteral("checkIntervalMinutes"))) {
        int minutes = obj.value(QStringLiteral("checkIntervalMinutes")).toInt(24 * 60);
        if (minutes < 1)
            minutes = 24 * 60;
        if (minutes % (7 * 24 * 60) == 0) {
            settings.checkUnit = "week";
            settings.checkEvery = minutes / (7 * 24 * 60);
        } else {
            settings.checkUnit = "day";
            settings.checkEvery = (minutes + (12 * 60)) / (24 * 60);
        }
    }
    if (settings.checkEvery < 1)
        settings.checkEvery = 1;
    if (settings.checkUnit == "week" && settings.checkEvery > 12)
        settings.checkEvery = 12;
    if (settings.checkUnit != "week" && settings.checkEvery > 30)
        settings.checkEvery = 30;
    settings.lastCheckEpoch = obj.value(QStringLiteral("lastCheckEpoch")).toVariant().toLongLong();
    if (settings.lastCheckEpoch < 0)
        settings.lastCheckEpoch = 0;
    return true;
}

bool JsonSettingsStore::save(const UpdaterSettings &settings)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("repository"), QString::fromStdString(settings.repositoryUrl));
    obj.insert(QStringLiteral("targetExecutable"), QString::fromStdString(settings.targetExecutable));
    obj.insert(QStringLiteral("token"), QString::fromStdString(settings.token));
    obj.insert(QStringLiteral("assetContains"), QString::fromStdString(settings.assetContains));
    obj.insert(QStringLiteral("scheduleEnabled"), settings.scheduleEnabled);
    obj.insert(QStringLiteral("checkEvery"), settings.checkEvery);
    obj.insert(QStringLiteral("checkUnit"), QString::fromStdString(settings.checkUnit));
    obj.insert(QStringLiteral("lastCheckEpoch"), static_cast<double>(settings.lastCheckEpoch));

    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    return file.commit();
}

std::string JsonSettingsStore::filePath() const
{
    return m_filePath.toStdString();
}
