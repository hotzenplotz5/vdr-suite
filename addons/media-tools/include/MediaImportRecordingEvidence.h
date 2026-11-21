#pragma once

#include <openssl/sha.h>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <cstdint>
#include <sstream>
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

inline constexpr const char* kMediaImportSourceEvidencePrefix =
    "media-import-source-sha256-v1:";

// Agent-local snapshot of the *executed* source, never a path authority.
// The source path is hashed together with the observed device, inode, size
// and nanosecond mtime. Only the digest crosses the Core boundary.
inline std::string mediaImportSourceEvidence(
    const std::filesystem::path& path, std::uint64_t device,
    std::uint64_t inode, std::uintmax_t bytes,
    std::int64_t seconds, std::int64_t nanoseconds)
{
    if (!path.is_absolute() || path.empty() ||
        path.lexically_normal() != path || device == 0 || inode == 0 ||
        bytes == 0 || nanoseconds < 0 || nanoseconds >= 1000000000)
        return {};
    std::ostringstream identity;
    identity << path.generic_string().size() << ':' << path.generic_string()
             << ':' << device << ':' << inode << ':' << bytes
             << ':' << seconds << ':' << nanoseconds;
    const auto input = identity.str();
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.data()),
           input.size(), digest);
    static constexpr char hex[] = "0123456789abcdef";
    std::string proof(kMediaImportSourceEvidencePrefix);
    for (unsigned char byte : digest) {
        proof.push_back(hex[byte >> 4]);
        proof.push_back(hex[byte & 15]);
    }
    return proof;
}

inline bool validMediaImportSourceEvidence(const std::string& proof)
{
    const std::string prefix(kMediaImportSourceEvidencePrefix);
    return proof.size() == prefix.size() + 64 &&
        proof.compare(0, prefix.size(), prefix) == 0 &&
        std::all_of(proof.begin() + static_cast<std::ptrdiff_t>(prefix.size()),
            proof.end(), [](unsigned char ch) {
                return (ch >= '0' && ch <= '9') ||
                       (ch >= 'a' && ch <= 'f');
            });
}

// Retain the original recording proof, adding an independently checked
// local source snapshot only for imports produced by the new Agent.
inline std::string mediaImportResultEvidence(
    const std::string& recording, const std::string& source)
{
    if (!validRecordingPathEvidence(recording) ||
        !validMediaImportSourceEvidence(source)) return {};
    return recording + "|" + source;
}

inline bool parseMediaImportResultEvidence(
    const std::string& input, std::string& recording,
    std::string& source)
{
    recording.clear();
    source.clear();
    if (validRecordingPathEvidence(input)) {
        recording = input; // Old journal entries remain readback-compatible.
        return true;
    }
    const auto split = input.find('|');
    if (split == std::string::npos ||
        input.find('|', split + 1) != std::string::npos) return false;
    recording = input.substr(0, split);
    source = input.substr(split + 1);
    if (validRecordingPathEvidence(recording) &&
        validMediaImportSourceEvidence(source)) return true;
    recording.clear();
    source.clear();
    return false;
}

// Core refresh scheduling accepts both the historical recording-only proof
 // and the current recording+source snapshot. Final success still requires a
 // fresh independent native VDR recording-cache readback.
inline bool validMediaImportResultRecordingEvidence(const std::string& input)
{
    std::string recording, source;
    return parseMediaImportResultEvidence(input, recording, source);
}

} // namespace vdrsuite::mediatools
