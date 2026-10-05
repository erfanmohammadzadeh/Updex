#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

inline std::wstring utf8ToWide(const std::string &text)
{
    if (text.empty())
        return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0)
        return {};
    std::wstring wide(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);
    return wide;
}

inline std::string wideToUtf8(const wchar_t *text, int length = -1)
{
    if (!text)
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, text, length, nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string utf8(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text, length, utf8.data(), size, nullptr, nullptr);
    while (!utf8.empty() && utf8.back() == '\0')
        utf8.pop_back();
    return utf8;
}

inline std::string wideToUtf8(const std::wstring &text)
{
    if (text.empty())
        return {};
    return wideToUtf8(text.c_str(), static_cast<int>(text.size()));
}
