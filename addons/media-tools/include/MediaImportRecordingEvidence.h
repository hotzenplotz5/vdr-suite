#pragma once

#include <openssl/sha.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <string>

namespace vdrsuite::mediatools {

// A digest of the handler's committed absolute VDR .rec directory.
// This is a correlation hint only; independent recording-cache readback
// and fresh successful inventory refresh are mandatory for success.
inline constexpr const char* kRecordingEvidencePrefix =
    "media-import-path-sha256-v1:";

inline std::string recordingPathEvidence(const std::string& absolutePath)
{
    if (absolutePath.empty() || absolutePath.front() != '/' ||
        absolutePath.size() > 4096 || absolutePath.find('\0') != std::string::npos)
        return {};
    const std::filesystem::path path(absolutePath);
    if (path.lexically_normal() != path || path.extension() != ".rec")
        return {};
    for (const auto& part : path)
        if (part == ".." || part == ".") return {};
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(absolutePath.data()),
           absolutePath.size(), digest);
    static constexpr char hex[] = "0123456789abcdef";
    std::string evidence(kRecordingEvidencePrefix);
    for (unsigned char byte : digest)
    {
        evidence.push_back(hex[byte >> 4]);
        evidence.push_back(hex[byte & 15]);
    }
    return evidence;
}

inline bool validRecordingPathEvidence(const std::string& text)
{
    const std::string prefix(kRecordingEvidencePrefix);
    return text.size() == prefix.size() + 64 &&
        text.compare(0, prefix.size(), prefix) == 0 &&
        std::all_of(text.begin() + static_cast<std::ptrdiff_t>(prefix.size()),
                    text.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
}
} // namespace vdrsuite::mediatools
