#pragma once

#include <QObject>
#include <QString>

class UpdateModel : public QObject
{
    Q_OBJECT

public:
    explicit UpdateModel(QObject *parent = nullptr);

    QString appPath() const;
    QString repository() const;
    QString token() const;
    QString currentVersion() const;
    QString latestVersion() const;
    QString status() const;
    int progress() const;
    bool busy() const;
    bool scheduleEnabled() const;
    int checkEvery() const;
    QString checkUnit() const;
    QString nextCheck() const;

    void setAppPath(const QString &appPath);
    void setRepository(const QString &repository);
    void setToken(const QString &token);
    void setCurrentVersion(const QString &version);
    void setLatestVersion(const QString &version);
    void setStatus(const QString &status);
    void setProgress(int progress);
    void setBusy(bool busy);
    void setScheduleEnabled(bool enabled);
    void setCheckEvery(int count);
    void setCheckUnit(const QString &unit);
    void setNextCheck(const QString &nextCheck);
    void appendLog(const QString &line);

signals:
    void changed();
    void logAppended(const QString &line);

private:
    QString m_appPath;
    QString m_repository;
    QString m_token;
    QString m_currentVersion = QStringLiteral("-");
    QString m_latestVersion = QStringLiteral("-");
    QString m_status = QStringLiteral("Ready");
    int m_progress = 0;
    bool m_busy = false;
    bool m_scheduleEnabled = false;
    int m_checkEvery = 1;
    QString m_checkUnit = QStringLiteral("day");
    QString m_nextCheck = QStringLiteral("Off");
};
