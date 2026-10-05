#include "infrastructure/GitHubRepoParser.h"

#include "application/Text.h"

namespace
{
std::string stripDecoration(std::string path)
{
    const auto cut = path.find_first_of("?#");
    if (cut != std::string::npos)
        path = path.substr(0, cut);
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
        path.pop_back();
    if (endsWithCi(path, ".git"))
        path.resize(path.size() - 4);
    while (!path.empty() && (path.back() == '/' || path.back() == '\\'))
        path.pop_back();
    return path;
}
}

std::optional<GitHubRepo> parseGitHubRepo(const std::string &text)
{
    std::string source = trimmed(text);
    if (source.empty())
        return std::nullopt;

    const std::string lower = toLowerCopy(source);
    std::string path;
    const std::string scp = "git@github.com:";
    if (lower.rfind(scp, 0) == 0) {
        path = source.substr(scp.size());
    } else {
        const auto host = lower.find("github.com/");
        if (host != std::string::npos) {
            path = source.substr(host + std::string("github.com/").size());
        } else if (source.find("://") == std::string::npos && source.find('@') == std::string::npos) {
            path = source;
        } else {
            return std::nullopt;
        }
    }

    path = stripDecoration(path);
    const auto slash = path.find('/');
    if (slash == std::string::npos || slash == 0)
        return std::nullopt;
    if (path.find('/', slash + 1) != std::string::npos)
        return std::nullopt;

    GitHubRepo repo;
    repo.owner = path.substr(0, slash);
    repo.name = path.substr(slash + 1);
    if (repo.owner.empty() || repo.name.empty())
        return std::nullopt;
    return repo;
}
