#pragma once

#include <algorithm>
#include <cctype>
#include <string>

inline std::string trimmed(std::string text)
{
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.front())))
        text.erase(text.begin());
    while (!text.empty() && isSpace(static_cast<unsigned char>(text.back())))
        text.pop_back();
    return text;
}

inline std::string toLowerCopy(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

inline bool endsWithCi(const std::string &text, const std::string &suffix)
{
    const std::string left = toLowerCopy(text);
    const std::string right = toLowerCopy(suffix);
    return left.size() >= right.size() && left.compare(left.size() - right.size(), right.size(), right) == 0;
}

inline bool containsCi(const std::string &text, const std::string &part)
{
    if (part.empty())
        return false;
    return toLowerCopy(text).find(toLowerCopy(part)) != std::string::npos;
}

inline std::string fileNameOf(const std::string &path)
{
    const auto pos = path.find_last_of("\\/");
    if (pos == std::string::npos)
        return path;
    return path.substr(pos + 1);
}

inline std::string fileStemOf(const std::string &path)
{
    std::string name = fileNameOf(path);
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos)
        return name;
    return name.substr(0, dot);
}
