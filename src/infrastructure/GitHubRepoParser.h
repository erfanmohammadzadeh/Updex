#pragma once

#include <optional>
#include <string>

struct GitHubRepo
{
    std::string owner;
    std::string name;
};

std::optional<GitHubRepo> parseGitHubRepo(const std::string &text);
