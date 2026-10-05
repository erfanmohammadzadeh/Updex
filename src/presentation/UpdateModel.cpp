#include "presentation/UpdateModel.h"

UpdateModel::UpdateModel(QObject *parent)
    : QObject(parent)
{
}

QString UpdateModel::appPath() const { return m_appPath; }
QString UpdateModel::repository() const { return m_repository; }
QString UpdateModel::token() const { return m_token; }
QString UpdateModel::currentVersion() const { return m_currentVersion; }
QString UpdateModel::latestVersion() const { return m_latestVersion; }
QString UpdateModel::status() const { return m_status; }
int UpdateModel::progress() const { return m_progress; }
bool UpdateModel::busy() const { return m_busy; }
bool UpdateModel::scheduleEnabled() const { return m_scheduleEnabled; }
int UpdateModel::checkEvery() const { return m_checkEvery; }
QString UpdateModel::checkUnit() const { return m_checkUnit; }
QString UpdateModel::nextCheck() const { return m_nextCheck; }

void UpdateModel::setAppPath(const QString &appPath)
{
    if (m_appPath == appPath)
        return;
    m_appPath = appPath;
    emit changed();
}

void UpdateModel::setRepository(const QString &repository)
{
    if (m_repository == repository)
        return;
    m_repository = repository;
    emit changed();
}

void UpdateModel::setToken(const QString &token)
{
    if (m_token == token)
        return;
    m_token = token;
    emit changed();
}

void UpdateModel::setCurrentVersion(const QString &version)
{
    if (m_currentVersion == version)
        return;
    m_currentVersion = version;
    emit changed();
}

void UpdateModel::setLatestVersion(const QString &version)
{
    if (m_latestVersion == version)
        return;
    m_latestVersion = version;
    emit changed();
}

void UpdateModel::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    emit changed();
}

void UpdateModel::setProgress(int progress)
{
    if (m_progress == progress)
        return;
    m_progress = progress;
    emit changed();
}

void UpdateModel::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit changed();
}

void UpdateModel::setScheduleEnabled(bool enabled)
{
    if (m_scheduleEnabled == enabled)
        return;
    m_scheduleEnabled = enabled;
    emit changed();
}

void UpdateModel::setCheckEvery(int count)
{
    if (m_checkEvery == count)
        return;
    m_checkEvery = count;
    emit changed();
}

void UpdateModel::setCheckUnit(const QString &unit)
{
    if (m_checkUnit == unit)
        return;
    m_checkUnit = unit;
    emit changed();
}

void UpdateModel::setNextCheck(const QString &nextCheck)
{
    if (m_nextCheck == nextCheck)
        return;
    m_nextCheck = nextCheck;
    emit changed();
}

void UpdateModel::appendLog(const QString &line)
{
    if (line.isEmpty())
        return;
    emit logAppended(line);
}
