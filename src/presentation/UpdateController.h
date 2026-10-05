#pragma once

#include "application/CheckForUpdate.h"
#include "application/ApplyUpdate.h"
#include "application/UpdateResults.h"
#include "application/UpdaterSettings.h"
#include "application/ports/IApplicationProbe.h"
#include "application/ports/IExecutableDiscovery.h"
#include "application/ports/ISettingsStore.h"
#include "presentation/UpdateModel.h"

#include <QObject>
#include <QString>
#include <QTimer>

class MainWindow;

class UpdateController : public QObject
{
    Q_OBJECT

public:
    UpdateController(UpdateModel &model,
                     CheckForUpdate &checkForUpdate,
                     ApplyUpdate &applyUpdate,
                     ISettingsStore &settingsStore,
                     IExecutableDiscovery &discovery,
                     IApplicationProbe &probe,
                     QObject *parent = nullptr);

    void attach(MainWindow *window);
    void start();

private:
    void onCheck();
    void onUpdate();
    void onSettingsEdited(const QString &appPath, const QString &repository, const QString &token, bool scheduleEnabled, int checkEvery, const QString &checkUnit);
    void onScheduledCheck();
    void reportProgress(int percent, const std::string &message);
    void runCheck(bool installIfAvailable);
    void armSchedule();
    void stampLastCheck();
    void presentCheck(const UpdateCheckResult &result);
    void confirmAndInstall();
    void runInstall();
    void refreshInstalledVersion();
    QString portableTarget(const QString &entered) const;
    std::string resolveExecutable(const UpdaterSettings &settings) const;

    UpdateModel &m_model;
    CheckForUpdate &m_check;
    ApplyUpdate &m_apply;
    ISettingsStore &m_store;
    IExecutableDiscovery &m_discovery;
    IApplicationProbe &m_probe;
    MainWindow *m_window = nullptr;
    UpdaterSettings m_settings;
    UpdateCheckResult m_lastCheck;
    bool m_hasCheck = false;
    QTimer m_scheduleTimer;
};
