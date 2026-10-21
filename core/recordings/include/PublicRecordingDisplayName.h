#pragma once

#include "VdrRecording.h"

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

// One server-owned Public-v1 presentation projection. It mirrors Web
// recordings2-shared.js recordingPathTitle/recordingNativeTitle and its
// decodeDisplayText semantics; native identifiers and paths are untouched.
namespace PublicRecordingDisplayName
{
struct Result
{
    std::string title;
    bool edited = false;
};

inline bool hexValue(char character, unsigned int& value)
{
    if (character >= '0' && character <= '9')
        value = static_cast<unsigned int>(character - '0');
    else if (character >= 'a' && character <= 'f')
        value = static_cast<unsigned int>(character - 'a' + 10);
    else if (character >= 'A' && character <= 'F')
        value = static_cast<unsigned int>(character - 'A' + 10);
    else return false;
    return true;
}

inline std::string decodeDisplayText(std::string value)
{
    for (int pass = 0; pass < 6; ++pass)
    {
        std::string decoded;
        decoded.reserve(value.size());
        for (std::size_t i = 0; i < value.size(); ++i)
        {
            unsigned int high = 0, low = 0;
            if (value[i] == '#' && i + 2U < value.size() &&
                hexValue(value[i + 1U], high) &&
                hexValue(value[i + 2U], low))
            {
                const auto scalar = (high << 4U) | low;
                if (scalar < 0x80U)
                    decoded.push_back(static_cast<char>(scalar));
                else
                {
                    // JS String.fromCharCode maps #XX to a Unicode
                    // codepoint; return valid UTF-8, not a Latin-1 byte.
                    decoded.push_back(static_cast<char>(0xc0U | (scalar >> 6U)));
                    decoded.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
                }
                i += 2U;
            }
            else decoded.push_back(value[i]);
        }
        if (decoded == value) break;
        value = std::move(decoded);
    }
    std::replace(value.begin(), value.end(), '_', ' ');
    return value;
}

inline std::string trim(const std::string& source)
{
    std::size_t begin = 0;
    while (begin < source.size() &&
           std::isspace(static_cast<unsigned char>(source[begin]))) ++begin;
    std::size_t end = source.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(source[end - 1U]))) --end;
    return source.substr(begin, end - begin);
}

inline std::vector<std::string> segments(std::string raw, bool nativeTitle)
{
    // Web: path normalizes *runs* of tildes; native title substitutes ~.
    std::replace(raw.begin(), raw.end(), '\\', '/');
    std::replace(raw.begin(), raw.end(), '~', '/');
    std::vector<std::string> parts;
    std::size_t start = 0U;
    while (start < raw.size())
    {
        const auto sep = raw.find('/', start);
        const std::string part = trim(raw.substr(
            start, sep == std::string::npos ? std::string::npos : sep - start));
        if (!part.empty()) parts.push_back(part);
        if (sep == std::string::npos) break;
        start = sep + 1U;
    }
    if (!nativeTitle && !parts.empty())
    {
        const std::string& last = parts.back();
        if (last.size() >= 4U)
        {
            std::string suffix = last.substr(last.size() - 4U);
            std::transform(suffix.begin(), suffix.end(), suffix.begin(),
                [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            if (suffix == ".rec") parts.pop_back();
        }
    }
    return parts;
}

inline Result project(const VdrRecording& recording)
{
    // Web first selects the last title directory of the native path,
    // dropping the final VDR .rec timestamp directory.
    // An opaque backendNativeId is not a path. Do not display it as a title
    // if the native title is available. Genuine VDR paths remain authoritative.
    std::string rawPath = recording.path;
    if (rawPath.empty() &&
        (recording.backendNativeId.find('/') != std::string::npos ||
         recording.backendNativeId.find('~') != std::string::npos))
        rawPath = recording.backendNativeId;
    auto parts = segments(rawPath, false);
    if (parts.empty()) parts = segments(recording.title, true);
    if (parts.empty()) return {"Aufnahme", false};

    // Decode AFTER segment selection: #2F inside a title is not a separator.
    std::string leaf = trim(decodeDisplayText(parts.back()));
    const bool edited = !leaf.empty() && leaf.front() == '%';
    // In VDR a leading '%' marks an edited cut. Strip only presentation
    // markers, never embedded % in a title, native path, or public ID.
    if (edited)
    {
        const auto visible = leaf.find_first_not_of('%');
        leaf = visible == std::string::npos ? "" : trim(leaf.substr(visible));
    }
    if (leaf.empty()) leaf = "Aufnahme";
    return {leaf, edited};
}
} // namespace PublicRecordingDisplayName
