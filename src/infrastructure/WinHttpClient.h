#pragma once

#include <functional>
#include <string>
#include <vector>

struct HttpResult
{
    bool ok = false;
    int status = 0;
    std::string body;
    std::string error;
};

struct HttpHeader
{
    std::string name;
    std::string value;
};

class WinHttpClient
{
public:
    HttpResult get(const std::string &url, const std::vector<HttpHeader> &headers) const;

    HttpResult download(const std::string &url,
                        const std::string &destinationPath,
                        const std::vector<HttpHeader> &headers,
                        const std::function<void(int percent)> &onProgress) const;
};
