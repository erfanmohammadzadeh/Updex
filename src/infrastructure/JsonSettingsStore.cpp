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
    return true;
}

bool JsonSettingsStore::save(const UpdaterSettings &settings)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("repository"), QString::fromStdString(settings.repositoryUrl));
    obj.insert(QStringLiteral("targetExecutable"), QString::fromStdString(settings.targetExecutable));
    obj.insert(QStringLiteral("token"), QString::fromStdString(settings.token));
    obj.insert(QStringLiteral("assetContains"), QString::fromStdString(settings.assetContains));

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
