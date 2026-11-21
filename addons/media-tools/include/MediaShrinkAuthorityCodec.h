#pragma once

#include "LocalMediaShrinkAgentExecutor.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <regex>
#include <string>

namespace vdrsuite::mediatools {
namespace authoritywire {

// Wire format is INTERNAL to the authenticated Backend Agent control plane.
// It is never a browser API response or an Agent command payload. Hex paths
// avoid using native path bytes as JSON syntax, log content, or shell input.
inline std::string encode(const std::string& input) {
    static const char* digits = "0123456789abcdef";
    std::string output;
    output.reserve(input.size() * 2);
    for (unsigned char byte : input) {
        output.push_back(digits[byte >> 4U]);
        output.push_back(digits[byte & 15U]);
    }
    return output;
}
inline bool decode(const std::string& input, std::string& output) {
    if (input.size() > 4096 || input.size() % 2 != 0) return false;
    output.clear();
    output.reserve(input.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < input.size(); i += 2) {
        const int hi = nibble(input[i]);
        const int lo = nibble(input[i + 1]);
        if (hi < 0 || lo < 0) return false;
        const char byte = static_cast<char>((hi << 4) | lo);
        if (byte == '\0' || byte == '\n' || byte == '\r') return false;
        output.push_back(byte);
    }
    return true;
}
inline bool identity(const std::string& value, std::size_t max) {
    return !value.empty() && value.size() <= max &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') ||
                   c == '_' || c == '-' || c == '.' || c == ':';
        });
}
inline bool valid(const LocalMediaShrinkAuthority& a) {
    if (!identity(a.actorId, 160) || !identity(a.backendId, 128) ||
        !shrinkOpaqueId(a.operationId, "op_") ||
        !shrinkOpaqueId(a.recordingId, "rec_") ||
        a.sourceFingerprint.size() != 26 ||
        a.sourceFingerprint.compare(0, 10, "shrink_v1_") != 0 ||
        !std::all_of(a.sourceFingerprint.begin() + 10,
                     a.sourceFingerprint.end(),
            [](unsigned char c) {
                return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            }) ||
        a.backendGeneration == 0 ||
        a.backendGeneration >
          static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()))
        return false;
    const auto root = a.videoRoot.string();
    const auto relative = a.relativeRecording.string();
    if (root.empty() || root.size() > 2048 ||
        relative.empty() || relative.size() > 2048 ||
        root.find_first_of("\r\n") != std::string::npos ||
        relative.find_first_of("\r\n") != std::string::npos ||
        !a.videoRoot.is_absolute() ||
        a.videoRoot.lexically_normal() != a.videoRoot ||
        a.relativeRecording.is_absolute() ||
        a.relativeRecording.lexically_normal() != a.relativeRecording ||
        a.relativeRecording.extension() != ".rec" ||
        a.relativeRecording.parent_path().empty())
        return false;
    return std::none_of(a.relativeRecording.begin(),
        a.relativeRecording.end(), [](const fs::path& part) {
            return part == ".." || part == ".";
        });
}
inline std::string serialize(const LocalMediaShrinkAuthority& a) {
    if (!valid(a)) return {};
    return "{\"actorId\":\"" + a.actorId +
        "\",\"backendId\":\"" + a.backendId +
        "\",\"operationId\":\"" + a.operationId +
        "\",\"recordingId\":\"" + a.recordingId +
        "\",\"sourceFingerprint\":\"" + a.sourceFingerprint +
        "\",\"backendGeneration\":" + std::to_string(a.backendGeneration) +
        ",\"videoRootHex\":\"" + encode(a.videoRoot.string()) +
        "\",\"relativeRecordingHex\":\"" +
        encode(a.relativeRecording.string()) + "\"}";
}
inline std::optional<LocalMediaShrinkAuthority> parse(
    const std::string& input)
{
    if (input.empty() || input.size() > 10000) return std::nullopt;
    static const std::regex shape(
        R"AUTH(^\{"actorId":"([A-Za-z0-9_.:-]{1,160})","backendId":"([A-Za-z0-9_.:-]{1,128})","operationId":"(op_[0-9a-f]{32})","recordingId":"(rec_[0-9a-f]{32})","sourceFingerprint":"(shrink_v1_[0-9a-f]{16})","backendGeneration":([1-9][0-9]{0,18}),"videoRootHex":"([0-9a-f]{2,4096})","relativeRecordingHex":"([0-9a-f]{2,4096})"\}$)AUTH");
    std::smatch m;
    if (!std::regex_match(input, m, shape)) return std::nullopt;
    LocalMediaShrinkAuthority a;
    a.actorId = m[1].str();
    a.backendId = m[2].str();
    a.operationId = m[3].str();
    a.recordingId = m[4].str();
    a.sourceFingerprint = m[5].str();
    try { a.backendGeneration = std::stoull(m[6].str()); }
    catch (...) { return std::nullopt; }
    std::string root;
    std::string relative;
    if (!decode(m[7].str(), root) || !decode(m[8].str(), relative))
        return std::nullopt;
    a.videoRoot = fs::path(root);
    a.relativeRecording = fs::path(relative);
    if (!valid(a)) return std::nullopt;
    return a;
}
} // namespace authoritywire
} // namespace vdrsuite::mediatools
