#include "infrastructure/GitHubReleaseRepository.h"

#include "application/Text.h"
#include "infrastructure/GitHubRepoParser.h"
#include "infrastructure/HttpClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace
{
std::string normalizeToken(std::string token)
{
    token = trimmed(token);
    const std::string lower = toLowerCopy(token);
    if (lower.rfind("bearer ", 0) == 0)
        token = trimmed(token.substr(7));
    else if (lower.rfind("token ", 0) == 0)
        token = trimmed(token.substr(6));
    return token;
}

QString githubMessage(const std::string &body)
{
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    if (!doc.isObject())
        return {};
    return doc.object().value(QStringLiteral("message")).toString();
}
}

ReleaseFetchResult GitHubReleaseRepository::fetchLatest(const std::string &repositoryUrl, const std::string &token) const
{
    ReleaseFetchResult result;
    const auto repo = parseGitHubRepo(repositoryUrl);
    if (!repo) {
        result.message = "Use a GitHub repository such as https://github.com/owner/name.";
        return result;
    }

    const QString url = QStringLiteral("https://api.github.com/repos/%1/%2/releases/latest")
                            .arg(QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(repo->owner))),
                                 QString::fromUtf8(QUrl::toPercentEncoding(QString::fromStdString(repo->name))));

    std::vector<HttpHeader> headers = {
        {"User-Agent", "Updex"},
        {"Accept", "application/vnd.github+json"},
        {"X-GitHub-Api-Version", "2022-11-28"}
    };
    const std::string secret = normalizeToken(token);
    if (!secret.empty())
        headers.push_back({"Authorization", "Bearer " + secret});

    const HttpResult response = HttpClient().get(url.toStdString(), headers);
    if (!response.ok) {
        const QString detail = githubMessage(response.body);
        if (response.status == 404) {
            result.message = "No GitHub release was found. Check the repository URL, or add a token if it is private.";
            if (!detail.isEmpty())
                result.message += " (" + detail.toStdString() + ")";
        } else if (!detail.isEmpty()) {
            result.message = detail.toStdString();
        } else if (!response.error.empty()) {
            result.message = response.error;
        } else {
            result.message = "Could not reach GitHub.";
        }
        return result;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(response.body));
    if (!doc.isObject()) {
        result.message = "GitHub returned an unexpected response.";
        return result;
    }

    const QJsonObject obj = doc.object();
    result.release.tag = obj.value(QStringLiteral("tag_name")).toString().toStdString();
    result.release.name = obj.value(QStringLiteral("name")).toString().toStdString();
    QString notes = obj.value(QStringLiteral("body")).toString();
    if (notes.size() > 2000)
        notes.resize(2000);
    result.release.notes = notes.toStdString();

    if (const auto version = Version::parse(result.release.tag)) {
        result.release.version = *version;
        result.release.hasVersion = true;
    } else if (const auto named = Version::parse(result.release.name)) {
        result.release.version = *named;
        result.release.hasVersion = true;
    }

    const QJsonArray assets = obj.value(QStringLiteral("assets")).toArray();
    for (const QJsonValue &value : assets) {
        const QJsonObject assetObj = value.toObject();
        Asset asset;
        asset.name = assetObj.value(QStringLiteral("name")).toString().toStdString();
        asset.downloadUrl = assetObj.value(QStringLiteral("browser_download_url")).toString().toStdString();
        asset.sizeBytes = assetObj.value(QStringLiteral("size")).toVariant().toLongLong();
        if (asset.name.empty() || asset.downloadUrl.empty())
            continue;
        result.release.assets.push_back(std::move(asset));
    }

    result.success = true;
    return result;
}
