#include "infrastructure/HttpClient.h"

#include <QEventLoop>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace
{
QNetworkRequest buildRequest(const std::string &url, const std::vector<HttpHeader> &headers)
{
    QNetworkRequest request{QUrl(QString::fromStdString(url))};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setMaximumRedirectsAllowed(8);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    request.setTransferTimeout(120000);
#endif
    for (const HttpHeader &header : headers)
        request.setRawHeader(QByteArray::fromStdString(header.name), QByteArray::fromStdString(header.value));
    return request;
}

bool waitForReply(QNetworkReply *reply, int timeoutMs)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();
    if (!reply->isFinished()) {
        reply->abort();
        return false;
    }
    return true;
}

HttpResult resultFromReply(QNetworkReply *reply, const QByteArray &body)
{
    HttpResult result;
    result.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    result.body = body.toStdString();
    if (reply->error() == QNetworkReply::NoError && result.status == 200) {
        result.ok = true;
        return result;
    }
    if (result.status != 0)
        result.error = "HTTP " + std::to_string(result.status);
    else
        result.error = reply->errorString().toStdString();
    if (result.error.empty())
        result.error = "Could not reach GitHub.";
    return result;
}
}

HttpResult HttpClient::get(const std::string &url, const std::vector<HttpHeader> &headers) const
{
    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(buildRequest(url, headers));
    if (!waitForReply(reply, 120000)) {
        HttpResult result;
        result.error = "GitHub request timed out.";
        return result;
    }
    return resultFromReply(reply, reply->readAll());
}

HttpResult HttpClient::download(const std::string &url,
                                const std::string &destinationPath,
                                const std::vector<HttpHeader> &headers,
                                const std::function<void(int percent)> &onProgress) const
{
    QFile file(QString::fromStdString(destinationPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        HttpResult result;
        result.error = "Could not create the download file.";
        return result;
    }

    QNetworkAccessManager manager;
    QNetworkReply *reply = manager.get(buildRequest(url, headers));
    int lastPercent = -1;
    QObject::connect(reply, &QNetworkReply::readyRead, reply, [&file, reply]() {
        file.write(reply->readAll());
    });
    QObject::connect(reply, &QNetworkReply::downloadProgress, reply, [&onProgress, &lastPercent](qint64 received, qint64 total) {
        if (!onProgress || total <= 0)
            return;
        int percent = static_cast<int>((received * 100) / total);
        if (percent > 100)
            percent = 100;
        if (percent == lastPercent)
            return;
        lastPercent = percent;
        onProgress(percent);
    });

    HttpResult result;
    if (!waitForReply(reply, 300000)) {
        result.error = "Download timed out.";
    } else {
        file.write(reply->readAll());
        result = resultFromReply(reply, {});
    }
    file.close();
    if (!result.ok)
        QFile::remove(QString::fromStdString(destinationPath));
    return result;
}
