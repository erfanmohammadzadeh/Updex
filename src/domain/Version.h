#pragma once

#include <optional>
#include <string>

struct Version
{
    int major = 0;
    int minor = 0;
    int patch = 0;
    int build = 0;
    int componentCount = 3;

    std::string toString() const;
    int compare(const Version &other) const;
    static std::optional<Version> parse(const std::string &text);
};
