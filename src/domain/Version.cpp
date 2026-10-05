#include "domain/Version.h"

#include <cctype>

std::string Version::toString() const
{
    std::string text = std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
    if (componentCount >= 4 || build != 0)
        text += "." + std::to_string(build);
    return text;
}

int Version::compare(const Version &other) const
{
    const int left[] = {major, minor, patch, build};
    const int right[] = {other.major, other.minor, other.patch, other.build};
    for (int index = 0; index < 4; ++index) {
        if (left[index] < right[index])
            return -1;
        if (left[index] > right[index])
            return 1;
    }
    return 0;
}

std::optional<Version> Version::parse(const std::string &text)
{
    const int length = static_cast<int>(text.size());
    for (int index = 0; index < length; ++index) {
        if (!std::isdigit(static_cast<unsigned char>(text[index])))
            continue;
        if (index > 0 && std::isdigit(static_cast<unsigned char>(text[index - 1])))
            continue;

        int parts[4] = {0, 0, 0, 0};
        int count = 0;
        int pos = index;
        bool valid = true;
        while (count < 4 && pos < length && std::isdigit(static_cast<unsigned char>(text[pos]))) {
            int value = 0;
            while (pos < length && std::isdigit(static_cast<unsigned char>(text[pos]))) {
                const int digit = text[pos] - '0';
                if (value > 100000000) {
                    valid = false;
                    break;
                }
                value = value * 10 + digit;
                ++pos;
            }
            if (!valid)
                break;
            parts[count++] = value;
            if (count < 4 && pos < length && text[pos] == '.') {
                ++pos;
                if (pos >= length || !std::isdigit(static_cast<unsigned char>(text[pos]))) {
                    valid = false;
                    break;
                }
            } else {
                break;
            }
        }

        if (valid && count >= 2) {
            Version version;
            version.major = parts[0];
            version.minor = parts[1];
            version.patch = parts[2];
            version.build = parts[3];
            version.componentCount = count;
            return version;
        }
    }
    return std::nullopt;
}
