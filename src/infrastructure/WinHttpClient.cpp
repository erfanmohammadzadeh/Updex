#include "infrastructure/HttpClient.h"

#include "infrastructure/TextConvert.h"

#include <QFile>
#include <QString>

#include <winhttp.h>

#include <cwchar>
#include <cwctype>
#include <optional>

namespace
{
struct WinHttpHandle
{
    HINTERNET handle = nullptr;
    ~WinHttpHandle()
    {
        if (handle)
            WinHttpCloseHandle(handle);
    }
    WinHttpHandle() = default;
    WinHttpHandle(const WinHttpHandle &) = delete;
    WinHttpHandle &operator=(const WinHttpHandle &) = delete;
};

std::wstring lowerWide(std::wstring text)
{
    for (wchar_t &c : text)
        c = static_cast<wchar_t>(towlower(c));
    return text;
}

std::string trimMessage(std::string text)
{
    while (!text.empty() && (text.back() == '\r' || text.back() == '\n' || text.back() == ' '))
        text.pop_back();
    return text;
}

std::string lastWinHttpError(const char *action)
{
    const DWORD code = GetLastError();
    wchar_t *message = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr,
                   code,
                   0,
                   reinterpret_cast<LPWSTR>(&message),
                   0,
                   nullptr);
    std::string text = std::string(action) + " (" + std::to_string(code) + ")";
    if (message) {
        text += ": ";
        text += trimMessage(wideToUtf8(message));
        LocalFree(message);
    }
    return text;
}

struct UrlParts
{
    std::wstring host;
    std::wstring path;
    INTERNET_PORT port = 0;
    bool https = true;
};

std::optional<UrlParts> crackUrl(const std::string &url)
{
    const std::wstring wide = utf8ToWide(url);
    if (wide.empty())
        return std::nullopt;

    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    wchar_t host[512];
    wchar_t path[4096];
    wchar_t extra[4096];
    parts.lpszHostName = host;
    parts.dwHostNameLength = sizeof(host) / sizeof(wchar_t);
    parts.lpszUrlPath = path;
    parts.dwUrlPathLength = sizeof(path) / sizeof(wchar_t);
    parts.lpszExtraInfo = extra;
    parts.dwExtraInfoLength = sizeof(extra) / sizeof(wchar_t);

    if (!WinHttpCrackUrl(wide.c_str(), static_cast<DWORD>(wide.size()), 0, &parts))
        return std::nullopt;

    UrlParts out;
    out.host.assign(parts.lpszHostName, parts.dwHostNameLength);
    out.path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    if (parts.dwExtraInfoLength > 0)
        out.path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);
    if (out.path.empty())
        out.path = L"/";
    out.port = parts.nPort;
    out.https = parts.nScheme == INTERNET_SCHEME_HTTPS;
    return out;
}

bool isGitHubHost(const std::wstring &host)
{
    const std::wstring lower = lowerWide(host);
    return lower == L"github.com" || lower == L"www.github.com" || lower == L"api.github.com";
}

std::wstring queryHeader(HINTERNET request, DWORD info)
{
    DWORD bytes = 0;
    WinHttpQueryHeaders(request, info, WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &bytes, WINHTTP_NO_HEADER_INDEX);
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER || bytes == 0)
        return {};
    std::wstring value(bytes / sizeof(wchar_t), L'\0');
    if (!WinHttpQueryHeaders(request, info, WINHTTP_HEADER_NAME_BY_INDEX, value.data(), &bytes, WINHTTP_NO_HEADER_INDEX))
        return {};
    while (!value.empty() && value.back() == L'\0')
        value.pop_back();
    return value;
}

int queryStatus(HINTERNET request)
{
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (!WinHttpQueryHeaders(request,
                             WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX,
                             &status,
                             &size,
                             WINHTTP_NO_HEADER_INDEX)) {
        return 0;
    }
    return static_cast<int>(status);
}

std::string toLowerAscii(const std::string &text)
{
    std::string out = text;
    for (char &c : out) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

std::string resolveRedirect(const std::string &current, const std::string &location)
{
    if (location.rfind("https://", 0) == 0 || location.rfind("http://", 0) == 0)
        return location;
    const auto scheme = current.find("://");
    if (scheme == std::string::npos)
        return location;
    const auto hostStart = scheme + 3;
    const auto pathStart = current.find('/', hostStart);
    const std::string origin = pathStart == std::string::npos ? current : current.substr(0, pathStart);
    if (!location.empty() && location[0] == '/')
        return origin + location;
    const auto slash = current.find_last_of('/');
    if (slash == std::string::npos)
        return origin + "/" + location;
    return current.substr(0, slash + 1) + location;
}

std::wstring headerBlock(const std::vector<HttpHeader> &headers, bool includeAuthorization)
{
    std::wstring block;
    for (const HttpHeader &header : headers) {
        if (!includeAuthorization && toLowerAscii(header.name) == "authorization")
            continue;
        block += utf8ToWide(header.name + ": " + header.value);
        block += L"\r\n";
    }
    return block;
}

using BodyWriter = std::function<bool(const char *data, size_t size)>;

HttpResult perform(const std::string &url,
                   const std::vector<HttpHeader> &headers,
                   const BodyWriter &writeBody,
                   const std::function<void(long long, long long)> &onBytes,
                   size_t maxBody)
{
    HttpResult result;
    std::string current = url;

    WinHttpHandle session;
    session.handle = WinHttpOpen(L"Updex",
                                 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                 WINHTTP_NO_PROXY_NAME,
                                 WINHTTP_NO_PROXY_BYPASS,
                                 0);
    if (!session.handle) {
        result.error = lastWinHttpError("Could not open HTTP");
        return result;
    }

    WinHttpSetTimeouts(session.handle, 15000, 15000, 30000, 120000);
#ifdef WINHTTP_OPTION_SECURE_PROTOCOLS
    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
#ifdef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
    protocols |= WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
#endif
    WinHttpSetOption(session.handle, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));
#endif

    for (int hop = 0; hop < 8; ++hop) {
        const auto parts = crackUrl(current);
        if (!parts) {
            result.error = "The download URL is invalid.";
            return result;
        }

        WinHttpHandle connection;
        connection.handle = WinHttpConnect(session.handle, parts->host.c_str(), parts->port, 0);
        if (!connection.handle) {
            result.error = lastWinHttpError("Could not connect");
            return result;
        }

        const DWORD flags = parts->https ? WINHTTP_FLAG_SECURE : 0;
        WinHttpHandle request;
        request.handle = WinHttpOpenRequest(connection.handle,
                                            L"GET",
                                            parts->path.c_str(),
                                            nullptr,
                                            WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            flags);
        if (!request.handle) {
            result.error = lastWinHttpError("Could not open request");
            return result;
        }

        DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
        WinHttpSetOption(request.handle, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));

        const std::wstring extra = headerBlock(headers, isGitHubHost(parts->host));
        if (!extra.empty()) {
            WinHttpAddRequestHeaders(request.handle,
                                     extra.c_str(),
                                     static_cast<DWORD>(-1),
                                     WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
        }

        if (!WinHttpSendRequest(request.handle, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
            || !WinHttpReceiveResponse(request.handle, nullptr)) {
            result.error = lastWinHttpError("GitHub request failed");
            return result;
        }

        result.status = queryStatus(request.handle);
        if (result.status == 301 || result.status == 302 || result.status == 303 || result.status == 307 || result.status == 308) {
            const std::string location = trimMessage(wideToUtf8(queryHeader(request.handle, WINHTTP_QUERY_LOCATION)));
            if (location.empty()) {
                result.error = "GitHub redirected without a location.";
                return result;
            }
            current = resolveRedirect(current, location);
            continue;
        }

        const std::wstring lengthHeader = queryHeader(request.handle, WINHTTP_QUERY_CONTENT_LENGTH);
        long long total = -1;
        if (!lengthHeader.empty())
            total = std::wcstoll(lengthHeader.c_str(), nullptr, 10);

        long long received = 0;
        std::string errorBody;
        for (;;) {
            DWORD available = 0;
            if (!WinHttpQueryDataAvailable(request.handle, &available)) {
                result.error = lastWinHttpError("Could not read the response");
                return result;
            }
            if (available == 0)
                break;
            std::string chunk(available, '\0');
            DWORD read = 0;
            if (!WinHttpReadData(request.handle, chunk.data(), available, &read)) {
                result.error = lastWinHttpError("Could not read the response");
                return result;
            }
            chunk.resize(read);
            received += read;
            if (onBytes)
                onBytes(received, total);

            if (result.status == 200) {
                if (maxBody > 0 && static_cast<size_t>(received) > maxBody) {
                    result.error = "The GitHub response is too large.";
                    return result;
                }
                if (writeBody && !writeBody(chunk.data(), chunk.size())) {
                    result.error = "Could not store the downloaded file.";
                    return result;
                }
            } else if (errorBody.size() < 8192) {
                errorBody.append(chunk);
            }
        }

        if (result.status != 200) {
            result.body = errorBody;
            result.error = "HTTP " + std::to_string(result.status);
            return result;
        }

        result.ok = true;
        return result;
    }

    result.error = "Too many redirects.";
    return result;
}
}

HttpResult HttpClient::get(const std::string &url, const std::vector<HttpHeader> &headers) const
{
    HttpResult result;
    result.body.clear();
    auto written = perform(
        url,
        headers,
        [&result](const char *data, size_t size) {
            result.body.append(data, size);
            return true;
        },
        {},
        2 * 1024 * 1024);
    if (!written.ok) {
        written.body = result.body.empty() ? written.body : result.body;
        return written;
    }
    written.body = result.body;
    return written;
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

    int lastPercent = -1;
    HttpResult result = perform(
        url,
        headers,
        [&file](const char *data, size_t size) {
            return file.write(data, static_cast<qint64>(size)) == static_cast<qint64>(size);
        },
        [&onProgress, &lastPercent](long long received, long long total) {
            if (!onProgress || total <= 0)
                return;
            int percent = static_cast<int>((received * 100) / total);
            if (percent > 100)
                percent = 100;
            if (percent == lastPercent)
                return;
            lastPercent = percent;
            onProgress(percent);
        },
        0);

    file.close();
    if (!result.ok)
        QFile::remove(QString::fromStdString(destinationPath));
    return result;
}
