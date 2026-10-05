#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include "infrastructure/ZipPackageInstaller.h"

#include "application/Text.h"
#include "infrastructure/WinHttpClient.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <tlhelp32.h>

#include <vector>

namespace
{
struct PlannedFile
{
    QString source;
    QString destination;
    QString relative;
};

struct RollbackItem
{
    QString destination;
    QString backup;
};

QString systemTool(const QString &relativePath)
{
    BOOL wow64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &wow64) && wow64)
        return QStringLiteral("C:/Windows/Sysnative/") + relativePath;
    return relativePath;
}

QString sanitizeName(QString name)
{
    for (QChar &c : name) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('.') && c != QLatin1Char('-') && c != QLatin1Char('_'))
            c = QLatin1Char('_');
    }
    if (name.isEmpty())
        name = QStringLiteral("package");
    return name;
}

int closeProgram(const QString &executablePath)
{
    const QString target = QFileInfo(executablePath).absoluteFilePath();
    const QString image = QFileInfo(target).fileName();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return 0;

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    std::vector<DWORD> pids;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (QString::fromWCharArray(entry.szExeFile).compare(image, Qt::CaseInsensitive) != 0)
                continue;
            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (!process)
                continue;
            wchar_t buffer[32768];
            DWORD size = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
            bool match = false;
            if (QueryFullProcessImageNameW(process, 0, buffer, &size))
                match = QString::fromWCharArray(buffer).compare(target, Qt::CaseInsensitive) == 0;
            if (match)
                pids.push_back(entry.th32ProcessID);
            CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);

    int closed = 0;
    for (DWORD pid : pids) {
        HANDLE process = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pid);
        if (!process)
            continue;
        if (TerminateProcess(process, 0)) {
            WaitForSingleObject(process, 5000);
            ++closed;
        }
        CloseHandle(process);
    }
    return closed;
}

void forceCloseByImage(const QString &executablePath)
{
    QProcess::execute(QStringLiteral("taskkill"),
                      {QStringLiteral("/F"), QStringLiteral("/IM"), QFileInfo(executablePath).fileName()});
}

bool extractZip(const QString &zipPath, const QString &destination, QString *error)
{
    QDir().mkpath(destination);

    auto run = [](const QString &program, const QStringList &arguments, QString *stdErr) {
        QProcess process;
        process.setProgram(program);
        process.setArguments(arguments);
        process.start();
        if (!process.waitForStarted(8000))
            return false;
        if (!process.waitForFinished(300000)) {
            process.kill();
            process.waitForFinished(3000);
            return false;
        }
        *stdErr = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    };

    QString stdErr;
    if (run(systemTool(QStringLiteral("tar.exe")),
            {QStringLiteral("-xf"), QDir::toNativeSeparators(zipPath), QStringLiteral("-C"), QDir::toNativeSeparators(destination)},
            &stdErr)) {
        return true;
    }

    const QString zipLiteral = QDir::toNativeSeparators(zipPath).replace(QLatin1Char('\''), QStringLiteral("''"));
    const QString destLiteral = QDir::toNativeSeparators(destination).replace(QLatin1Char('\''), QStringLiteral("''"));
    const QString command = QStringLiteral("Expand-Archive -LiteralPath '%1' -DestinationPath '%2' -Force").arg(zipLiteral, destLiteral);
    QString powerErr;
    if (run(systemTool(QStringLiteral("WindowsPowerShell/v1.0/powershell.exe")),
            {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"), QStringLiteral("-Command"), command},
            &powerErr)) {
        return true;
    }

    *error = stdErr.isEmpty() ? powerErr : stdErr;
    if (error->isEmpty())
        *error = QStringLiteral("Could not extract the zip. Windows tar and PowerShell both failed.");
    return false;
}

QString contentRoot(const QString &extracted)
{
    const QFileInfoList entries = QDir(extracted).entryInfoList(QDir::Dirs | QDir::Files | QDir::Hidden | QDir::NoDotAndDotDot);
    if (entries.size() == 1 && entries.first().isDir())
        return entries.first().absoluteFilePath();
    return extracted;
}

bool shouldSkip(const QString &relative, const QString &destination, const QString &updaterPath)
{
    const QString name = QFileInfo(relative).fileName();
    if (name.compare(QStringLiteral("updex.json"), Qt::CaseInsensitive) == 0)
        return true;
    if (relative.startsWith(QStringLiteral("__MACOSX")))
        return true;
    if (relative.startsWith(QStringLiteral("updex-backup"), Qt::CaseInsensitive))
        return true;
    if (!updaterPath.isEmpty()
        && QFileInfo(destination).absoluteFilePath().compare(QFileInfo(updaterPath).absoluteFilePath(), Qt::CaseInsensitive) == 0) {
        return true;
    }
    return false;
}

void restore(const std::vector<RollbackItem> &items)
{
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        QFile::setPermissions(it->destination, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser);
        QFile::remove(it->destination);
        if (!it->backup.isEmpty())
            QFile::copy(it->backup, it->destination);
    }
}

bool replaceOne(const PlannedFile &file, const QString &backupRoot, std::vector<RollbackItem> *rollback, QString *error)
{
    QDir().mkpath(QFileInfo(file.destination).absolutePath());
    RollbackItem item;
    item.destination = file.destination;

    if (QFile::exists(file.destination)) {
        const QString backup = QDir(backupRoot).filePath(file.relative);
        QDir().mkpath(QFileInfo(backup).absolutePath());
        QFile::remove(backup);
        if (!QFile::copy(file.destination, backup)) {
            *error = QStringLiteral("Could not back up %1.").arg(file.relative);
            return false;
        }
        item.backup = backup;
    }
    rollback->push_back(item);

    if (QFile::exists(file.destination)) {
        QFile existing(file.destination);
        existing.setPermissions(existing.permissions() | QFile::WriteOwner | QFile::WriteUser);
        if (!existing.remove()) {
            const QString oldPath = file.destination + QStringLiteral(".old");
            QFile::remove(oldPath);
            if (!QFile::rename(file.destination, oldPath)) {
                *error = QStringLiteral("File is in use: %1.").arg(file.relative);
                return false;
            }
        }
    }

    if (!QFile::copy(file.source, file.destination)) {
        *error = QStringLiteral("Could not copy %1.").arg(file.relative);
        return false;
    }
    QFile copied(file.destination);
    copied.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser | QFile::ReadGroup | QFile::ReadOther);
    return true;
}

std::vector<HttpHeader> downloadHeaders(const std::string &token)
{
    std::vector<HttpHeader> headers = {
        {"User-Agent", "Updex"},
        {"Accept", "application/octet-stream"}
    };
    std::string secret = trimmed(token);
    const std::string lower = toLowerCopy(secret);
    if (lower.rfind("bearer ", 0) == 0)
        secret = trimmed(secret.substr(7));
    else if (lower.rfind("token ", 0) == 0)
        secret = trimmed(secret.substr(6));
    if (!secret.empty())
        headers.push_back({"Authorization", "Bearer " + secret});
    return headers;
}
}

ApplyUpdateResult ZipPackageInstaller::install(const InstallRequest &request, const InstallProgress &progress) const
{
    ApplyUpdateResult result;
    const QString executable = QString::fromStdString(request.application.executablePath);
    const QString appDir = QFileInfo(executable).absolutePath();
    const bool zip = endsWithCi(request.asset.name, ".zip");
    const QString tag = sanitizeName(QString::fromStdString(request.release.tag.empty() ? request.release.version.toString() : request.release.tag));
    const QString jobDir = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                               .filePath(QStringLiteral("Updex/%1-%2").arg(tag).arg(QDateTime::currentMSecsSinceEpoch()));
    QDir().mkpath(jobDir);

    const QString downloadPath = QDir(jobDir).filePath(sanitizeName(QString::fromStdString(request.asset.name)));
    if (progress)
        progress(0, "Downloading " + request.asset.name);

    const HttpResult downloaded = WinHttpClient().download(
        request.asset.downloadUrl,
        downloadPath.toStdString(),
        downloadHeaders(request.token),
        [&progress](int percent) {
            if (progress)
                progress(percent * 65 / 100, {});
        });
    if (!downloaded.ok) {
        result.message = downloaded.error.empty() ? "Download failed." : downloaded.error;
        return result;
    }

    std::vector<PlannedFile> files;
    if (zip) {
        if (progress)
            progress(70, "Extracting " + request.asset.name);
        const QString extracted = QDir(jobDir).filePath(QStringLiteral("extracted"));
        QString extractError;
        if (!extractZip(downloadPath, extracted, &extractError)) {
            result.message = extractError.toStdString();
            return result;
        }
        const QString root = contentRoot(extracted);
        QDirIterator iterator(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            QString relative = QDir(root).relativeFilePath(iterator.filePath());
            relative.replace(QLatin1Char('\\'), QLatin1Char('/'));
            if (relative == QLatin1String("..") || relative.startsWith(QLatin1String("../")) || relative.contains(QLatin1String("/../")))
                continue;
            PlannedFile file;
            file.source = iterator.filePath();
            file.relative = relative;
            file.destination = QDir(appDir).filePath(relative);
            if (shouldSkip(relative, file.destination, QString::fromStdString(request.updaterExecutablePath)))
                continue;
            files.push_back(file);
        }
    } else {
        PlannedFile file;
        file.source = downloadPath;
        file.destination = executable;
        file.relative = QFileInfo(executable).fileName();
        files.push_back(file);
    }

    if (files.empty()) {
        result.message = "The release package did not contain any files to copy.";
        return result;
    }

    if (progress)
        progress(76, "Closing the program if it is running");
    const int closed = closeProgram(executable);
    if (closed > 0) {
        if (progress)
            progress(78, "Closed " + std::to_string(closed) + " running instance(s).");
        Sleep(400);
    }

    const QString backupRoot = QDir(appDir).filePath(QStringLiteral("updex-backup/%1")
                                                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))));
    std::vector<RollbackItem> rollback;
    bool triedForceClose = false;
    for (int index = 0; index < static_cast<int>(files.size()); ++index) {
        QString error;
        if (!replaceOne(files[static_cast<size_t>(index)], backupRoot, &rollback, &error)) {
            if (!triedForceClose) {
                forceCloseByImage(executable);
                triedForceClose = true;
                Sleep(500);
            }
            error.clear();
            if (!replaceOne(files[static_cast<size_t>(index)], backupRoot, &rollback, &error)) {
                restore(rollback);
                result.message = error.toStdString() + " Previous files were restored.";
                return result;
            }
        }
        if (progress)
            progress(80 + ((index + 1) * 20 / static_cast<int>(files.size())), {});
    }

    for (const PlannedFile &file : files)
        QFile::remove(file.destination + QStringLiteral(".old"));

    QDir(jobDir).removeRecursively();
    result.success = true;
    result.filesReplaced = static_cast<int>(files.size());
    result.message = "Replaced " + std::to_string(result.filesReplaced) + " file(s).";
    if (QDir(backupRoot).exists()) {
        result.backupDirectory = QDir::toNativeSeparators(backupRoot).toStdString();
        result.message += " Backup: " + result.backupDirectory;
    }
    if (progress)
        progress(100, {});
    return result;
}
