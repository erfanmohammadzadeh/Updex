#include "application/CheckForUpdate.h"

#include "application/AssetSelector.h"
#include "application/Text.h"

namespace
{
std::string briefNotes(const std::string &notes)
{
    std::string out;
    int lines = 0;
    for (char c : notes) {
        if (c == '\r')
            continue;
        if (c == '\n') {
            ++lines;
            if (lines >= 3)
                break;
            if (!out.empty())
                out.push_back('\n');
            continue;
        }
        out.push_back(c);
        if (out.size() >= 300)
            break;
    }
    return trimmed(out);
}
}

CheckForUpdate::CheckForUpdate(const IReleaseRepository &releases, const IApplicationProbe &probe)
    : m_releases(releases)
    , m_probe(probe)
{
}

UpdateCheckResult CheckForUpdate::execute(const UpdaterSettings &settings, const std::string &resolvedExecutable) const
{
    UpdateCheckResult result;
    result.resolvedExecutable = resolvedExecutable;

    if (trimmed(resolvedExecutable).empty()) {
        result.message = "Choose the program exe that sits beside Updex.";
        return result;
    }

    const InstalledApplication app = m_probe.inspect(resolvedExecutable);
    if (app.executablePath.empty()) {
        result.message = "Program was not found: " + resolvedExecutable;
        return result;
    }

    result.resolvedExecutable = app.executablePath;
    result.hasLocalVersion = app.hasVersion;
    result.localVersion = app.version;

    if (trimmed(settings.repositoryUrl).empty()) {
        result.message = "Set the GitHub repository that publishes releases.";
        return result;
    }

    const ReleaseFetchResult fetched = m_releases.fetchLatest(settings.repositoryUrl, settings.token);
    if (!fetched.success) {
        result.message = fetched.message;
        return result;
    }

    result.success = true;
    result.hasLatest = true;
    result.latest = fetched.release;

    if (!fetched.release.hasVersion) {
        result.success = false;
        result.message = "The latest release tag \"" + fetched.release.tag + "\" has no version number such as 1.2.3.";
        return result;
    }

    const auto asset = selectReleaseAsset(fetched.release.assets, settings.assetContains, app.executablePath);
    const std::string latestText = fetched.release.version.toString();
    const bool newer = !app.hasVersion || app.version.compare(fetched.release.version) < 0;

    if (!newer) {
        if (app.version.compare(fetched.release.version) == 0)
            result.message = "Already up to date (" + app.version.toString() + ").";
        else
            result.message = "Installed version " + app.version.toString() + " is newer than release " + latestText + ".";
        return result;
    }

    result.updateAvailable = true;
    if (!asset) {
        result.message = "Release " + latestText + " is newer, but it has no .zip or program file. Attach a zip of the program files to the GitHub release.";
        return result;
    }

    result.selectedAsset = *asset;
    result.canUpdate = true;
    if (!app.hasVersion) {
        result.message = "Could not read the installed version. Release " + latestText
            + " can replace the program files. Asset: " + asset->name + ".";
    } else {
        result.message = "Update available: " + app.version.toString() + " -> " + latestText
            + ". Asset: " + asset->name + ".";
    }

    const std::string notes = briefNotes(fetched.release.notes);
    if (!notes.empty())
        result.message += "\n" + notes;
    return result;
}
