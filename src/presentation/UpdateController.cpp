#include "presentation/UpdateController.h"

#include "application/Text.h"
#include "presentation/mainwindow.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QPointer>
#include <QtConcurrent>

namespace
{
qint64 scheduleIntervalMs(const UpdaterSettings &settings)
{
    const qint64 dayMs = 24LL * 60 * 60 * 1000;
    const int count = settings.checkEvery < 1 ? 1 : settings.checkEvery;
    if (settings.checkUnit == "week")
        return count * 7 * dayMs;
    return count * dayMs;
}

QString schedulePhrase(const UpdaterSettings &settings)
{
    const int count = settings.checkEvery < 1 ? 1 : settings.checkEvery;
    const bool week = settings.checkUnit == "week";
    if (count == 1)
        return week ? QStringLiteral("every week") : QStringLiteral("every day");
    return QStringLiteral("every %1 %2").arg(count).arg(week ? QStringLiteral("weeks") : QStringLiteral("days"));
}
}

UpdateController::UpdateController(UpdateModel &model,
                                   CheckForUpdate &checkForUpdate,
                                   ApplyUpdate &applyUpdate,
                                   ISettingsStore &settingsStore,
                                   IExecutableDiscovery &discovery,
                                   IApplicationProbe &probe,
                                   QObject *parent)
    : QObject(parent)
    , m_model(model)
    , m_check(checkForUpdate)
    , m_apply(applyUpdate)
    , m_store(settingsStore)
    , m_discovery(discovery)
    , m_probe(probe)
    , m_scheduleTimer(this)
{
    m_scheduleTimer.setSingleShot(true);
    connect(&m_scheduleTimer, &QTimer::timeout, this, &UpdateController::onScheduledCheck);
}

void UpdateController::attach(MainWindow *window)
{
    m_window = window;
    connect(window, &MainWindow::checkRequested, this, &UpdateController::onCheck);
    connect(window, &MainWindow::updateRequested, this, &UpdateController::onUpdate);
    connect(window, &MainWindow::settingsEdited, this, &UpdateController::onSettingsEdited);
}

void UpdateController::start()
{
    std::string error;
    if (!m_store.load(m_settings, error))
        m_model.appendLog(QString::fromStdString(error));

    const QString appDir = QCoreApplication::applicationDirPath();
    const QString selfName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    if (m_settings.targetExecutable.empty()) {
        const std::vector<std::string> found = m_discovery.findExecutables(appDir.toStdString(), selfName.toStdString());
        if (found.size() == 1) {
            m_settings.targetExecutable = QFileInfo(QString::fromStdString(found.front())).fileName().toStdString();
            m_store.save(m_settings);
            m_model.appendLog(QStringLiteral("Found program beside Updex: %1")
                                  .arg(QString::fromStdString(m_settings.targetExecutable)));
        } else if (found.size() > 1) {
            m_model.appendLog(QStringLiteral("Several programs are beside Updex. Choose the one to update:"));
            for (const std::string &path : found)
                m_model.appendLog(QFileInfo(QString::fromStdString(path)).fileName());
        }
    }

    m_model.setAppPath(QString::fromStdString(m_settings.targetExecutable));
    m_model.setRepository(QString::fromStdString(m_settings.repositoryUrl));
    m_model.setToken(QString::fromStdString(m_settings.token));
    m_model.setScheduleEnabled(m_settings.scheduleEnabled);
    m_model.setCheckEvery(m_settings.checkEvery);
    m_model.setCheckUnit(QString::fromStdString(m_settings.checkUnit));
    refreshInstalledVersion();
    armSchedule();
    m_model.appendLog(QStringLiteral("Config: %1").arg(QString::fromStdString(m_store.filePath())));
    if (m_settings.scheduleEnabled) {
        m_model.appendLog(QStringLiteral("Scheduled check %1. %2")
                              .arg(schedulePhrase(m_settings), m_model.nextCheck()));
    } else {
        m_model.appendLog(QStringLiteral("Set the GitHub repository that publishes releases, then check for an update."));
    }
    m_model.setStatus(QStringLiteral("Ready"));
}

void UpdateController::onCheck()
{
    runCheck(false);
}

void UpdateController::onUpdate()
{
    if (m_model.busy())
        return;
    if (!m_hasCheck || !m_lastCheck.canUpdate)
        runCheck(true);
    else
        confirmAndInstall();
}

void UpdateController::onSettingsEdited(const QString &appPath, const QString &repository, const QString &token, bool scheduleEnabled, int checkEvery, const QString &checkUnit)
{
    if (m_model.busy())
        return;

    const std::string unit = checkUnit == QStringLiteral("week") ? "week" : "day";
    const int maxCount = unit == "week" ? 12 : 30;
    if (checkEvery < 1)
        checkEvery = 1;
    if (checkEvery > maxCount)
        checkEvery = maxCount;

    const std::string target = portableTarget(appPath).toStdString();
    const std::string repo = repository.trimmed().toStdString();
    const std::string secret = token.trimmed().toStdString();
    const bool pathChanged = target != m_settings.targetExecutable || repo != m_settings.repositoryUrl || secret != m_settings.token;
    const bool scheduleChanged = scheduleEnabled != m_settings.scheduleEnabled || checkEvery != m_settings.checkEvery || unit != m_settings.checkUnit;
    if (!pathChanged && !scheduleChanged)
        return;

    m_settings.targetExecutable = target;
    m_settings.repositoryUrl = repo;
    m_settings.token = secret;
    m_settings.scheduleEnabled = scheduleEnabled;
    m_settings.checkEvery = checkEvery;
    m_settings.checkUnit = unit;
    if (pathChanged)
        m_hasCheck = false;
    if (!m_store.save(m_settings))
        m_model.appendLog(QStringLiteral("Could not write updex.json."));

    m_model.setAppPath(QString::fromStdString(m_settings.targetExecutable));
    m_model.setRepository(QString::fromStdString(m_settings.repositoryUrl));
    m_model.setToken(QString::fromStdString(m_settings.token));
    m_model.setScheduleEnabled(m_settings.scheduleEnabled);
    m_model.setCheckEvery(m_settings.checkEvery);
    m_model.setCheckUnit(QString::fromStdString(m_settings.checkUnit));
    if (pathChanged) {
        m_model.setLatestVersion(QStringLiteral("-"));
        refreshInstalledVersion();
    }
    armSchedule();
    if (scheduleChanged && m_settings.scheduleEnabled) {
        m_model.appendLog(QStringLiteral("Scheduled check %1. %2")
                              .arg(schedulePhrase(m_settings), m_model.nextCheck()));
    } else if (scheduleChanged) {
        m_model.appendLog(QStringLiteral("Scheduled check is off."));
    }
}

void UpdateController::onScheduledCheck()
{
    if (!m_settings.scheduleEnabled) {
        armSchedule();
        return;
    }

    const qint64 elapsedMs = m_settings.lastCheckEpoch > 0
        ? (QDateTime::currentSecsSinceEpoch() - m_settings.lastCheckEpoch) * 1000
        : scheduleIntervalMs(m_settings);
    if (elapsedMs < scheduleIntervalMs(m_settings)) {
        armSchedule();
        return;
    }

    if (m_model.busy()) {
        m_scheduleTimer.start(60 * 1000);
        const QString when = QDateTime::currentDateTime().addSecs(60).toString(QStringLiteral("HH:mm:ss"));
        m_model.setNextCheck(QStringLiteral("Next: %1").arg(when));
        return;
    }
    runCheck(true);
}

void UpdateController::reportProgress(int percent, const std::string &message)
{
    m_model.setProgress(percent);
    if (message.empty())
        return;
    const QString text = QString::fromStdString(message);
    m_model.appendLog(text);
    m_model.setStatus(text.section(QLatin1Char('\n'), 0, 0));
}

void UpdateController::runCheck(bool installIfAvailable)
{
    if (m_model.busy())
        return;

    m_model.setBusy(true);
    m_model.setProgress(0);
    m_model.setStatus(QStringLiteral("Checking for updates"));
    m_model.appendLog(QStringLiteral("Checking GitHub releases..."));

    const UpdaterSettings settings = m_settings;
    const std::string executable = resolveExecutable(settings);
    auto *watcher = new QFutureWatcher<UpdateCheckResult>(this);
    connect(watcher, &QFutureWatcher<UpdateCheckResult>::finished, this, [this, watcher, installIfAvailable]() {
        UpdateCheckResult result;
        try {
            result = watcher->result();
        } catch (...) {
            result.message = "Check failed.";
        }
        watcher->deleteLater();
        m_lastCheck = result;
        m_hasCheck = true;
        presentCheck(result);
        stampLastCheck();
        armSchedule();
        m_model.setBusy(false);
        if (installIfAvailable && result.canUpdate)
            confirmAndInstall();
    });
    watcher->setFuture(QtConcurrent::run([this, settings, executable]() {
        return m_check.execute(settings, executable);
    }));
}

void UpdateController::presentCheck(const UpdateCheckResult &result)
{
    if (result.resolvedExecutable.empty())
        m_model.setCurrentVersion(QStringLiteral("-"));
    else if (result.hasLocalVersion)
        m_model.setCurrentVersion(QString::fromStdString(result.localVersion.toString()));
    else
        m_model.setCurrentVersion(QStringLiteral("Unknown"));

    if (result.hasLatest && result.latest.hasVersion)
        m_model.setLatestVersion(QString::fromStdString(result.latest.version.toString()));
    else
        m_model.setLatestVersion(QStringLiteral("-"));

    m_model.appendLog(QString::fromStdString(result.message));
    const QString summary = QString::fromStdString(result.message).section(QLatin1Char('\n'), 0, 0);
    m_model.setStatus(result.canUpdate ? QStringLiteral("Update available") : summary);
    if (result.success)
        m_model.setProgress(100);
}

void UpdateController::confirmAndInstall()
{
    if (!m_hasCheck || !m_lastCheck.canUpdate || !m_window)
        return;

    const QString version = QString::fromStdString(m_lastCheck.latest.version.toString());
    const QString target = QDir::toNativeSeparators(QString::fromStdString(m_lastCheck.resolvedExecutable));
    m_model.setBusy(true);
    const auto answer = QMessageBox::question(
        m_window,
        QStringLiteral("Update"),
        QStringLiteral("Install release %1 and replace files for:\n%2\n\nThe program will be closed if it is running.")
            .arg(version, target),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (answer != QMessageBox::Yes) {
        m_model.setBusy(false);
        m_model.setStatus(QStringLiteral("Update cancelled"));
        m_model.appendLog(QStringLiteral("Update cancelled."));
        return;
    }
    runInstall();
}

void UpdateController::runInstall()
{
    m_model.setBusy(true);
    m_model.setProgress(0);
    m_model.setStatus(QStringLiteral("Updating"));

    InstallRequest request;
    request.release = m_lastCheck.latest;
    request.asset = m_lastCheck.selectedAsset;
    request.application = m_probe.inspect(m_lastCheck.resolvedExecutable);
    request.token = m_settings.token;
    request.updaterExecutablePath = QCoreApplication::applicationFilePath().toStdString();

    QPointer<UpdateController> self(this);
    auto *watcher = new QFutureWatcher<ApplyUpdateResult>(this);
    connect(watcher, &QFutureWatcher<ApplyUpdateResult>::finished, this, [this, watcher]() {
        ApplyUpdateResult result;
        try {
            result = watcher->result();
        } catch (...) {
            result.message = "Update failed.";
        }
        watcher->deleteLater();
        m_model.appendLog(QString::fromStdString(result.message));
        if (result.success) {
            m_model.setProgress(100);
            m_model.setStatus(QStringLiteral("Updated"));
            m_hasCheck = false;
            refreshInstalledVersion();
        } else {
            m_model.setStatus(QStringLiteral("Update failed"));
        }
        m_model.setBusy(false);
    });
    watcher->setFuture(QtConcurrent::run([this, request, self]() {
        return m_apply.execute(request, [self](int percent, const std::string &message) {
            if (!self)
                return;
            QMetaObject::invokeMethod(self.data(), [self, percent, message]() {
                if (self)
                    self->reportProgress(percent, message);
            }, Qt::QueuedConnection);
        });
    }));
}

void UpdateController::refreshInstalledVersion()
{
    const std::string executable = resolveExecutable(m_settings);
    if (executable.empty() || !QFileInfo::exists(QString::fromStdString(executable))) {
        m_model.setCurrentVersion(QStringLiteral("-"));
        return;
    }
    const InstalledApplication app = m_probe.inspect(executable);
    m_model.setCurrentVersion(app.hasVersion ? QString::fromStdString(app.version.toString()) : QStringLiteral("Unknown"));
}

QString UpdateController::portableTarget(const QString &entered) const
{
    const QString trimmed = entered.trimmed();
    if (trimmed.isEmpty())
        return {};

    const QFileInfo info(trimmed);
    if (!info.isAbsolute())
        return trimmed;

    const QString appDir = QDir(QCoreApplication::applicationDirPath()).absolutePath();
    if (QFileInfo(info.absolutePath()).absoluteFilePath().compare(appDir, Qt::CaseInsensitive) == 0)
        return info.fileName();
    return info.absoluteFilePath();
}

void UpdateController::armSchedule()
{
    if (!m_settings.scheduleEnabled) {
        m_scheduleTimer.stop();
        m_model.setNextCheck(QStringLiteral("Off"));
        return;
    }

    const qint64 intervalMs = scheduleIntervalMs(m_settings);
    qint64 delayMs = 5000;
    if (m_settings.lastCheckEpoch > 0) {
        const qint64 elapsedMs = (QDateTime::currentSecsSinceEpoch() - m_settings.lastCheckEpoch) * 1000;
        delayMs = intervalMs - elapsedMs;
        if (delayMs < 1000)
            delayMs = 1000;
    }
    if (delayMs > 2147483647LL)
        delayMs = 2147483647LL;

    m_scheduleTimer.start(static_cast<int>(delayMs));
    const QString format = delayMs < 60000 ? QStringLiteral("HH:mm:ss") : QStringLiteral("yyyy-MM-dd HH:mm");
    m_model.setNextCheck(QStringLiteral("Next: %1").arg(QDateTime::currentDateTime().addMSecs(static_cast<int>(delayMs)).toString(format)));
}

void UpdateController::stampLastCheck()
{
    m_settings.lastCheckEpoch = QDateTime::currentSecsSinceEpoch();
    if (!m_store.save(m_settings))
        m_model.appendLog(QStringLiteral("Could not write updex.json."));
}

std::string UpdateController::resolveExecutable(const UpdaterSettings &settings) const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString selfName = QFileInfo(QCoreApplication::applicationFilePath()).fileName();
    if (trimmed(settings.targetExecutable).empty()) {
        const std::vector<std::string> found = m_discovery.findExecutables(appDir.toStdString(), selfName.toStdString());
        if (found.size() == 1)
            return found.front();
        return {};
    }

    QFileInfo info(QString::fromStdString(settings.targetExecutable));
    if (info.isRelative())
        info.setFile(QDir(appDir).filePath(info.filePath()));
    return info.absoluteFilePath().toStdString();
}
