#pragma once

#include <cstdint>
#include <limits>
#include <regex>
#include <string>

namespace vdrsuite::addons {
struct ActivationHttpMutation {
    std::string backendId;
    bool enabled = false;
    std::int64_t expectedRevision = 0;
};
// Exact, bounded wire schema. The browser cannot supply RuntimeEvidence,
// Agent identities, handler hashes, paths or executor settings.
inline bool parseActivationHttpMutation(
    const std::string& body, ActivationHttpMutation& out)
{
    if (body.size() > 512U) return false;
    static const std::regex canonical(
        R"ACT(^\s*\{\s*"backendId"\s*:\s*"([a-zA-Z0-9_.:-]{1,128})"\s*,\s*"desiredEnabled"\s*:\s*(true|false)\s*,\s*"expectedRevision"\s*:\s*(0|[1-9][0-9]{0,16})\s*\}\s*$)ACT");
    std::smatch match;
    if (!std::regex_match(body, match, canonical)) return false;
    const std::string backend = match[1].str();
    if (backend == "." || backend == ".." || backend == "*") return false;
    std::int64_t revision = 0;
    for (const char c : match[3].str()) {
        const int digit = c - '0';
        if (revision > (std::numeric_limits<std::int64_t>::max() - digit) / 10)
            return false;
        revision = revision * 10 + digit;
    }
    out = {backend, match[2].str() == "true", revision};
    return true;
}
// Exact read query, no duplicate params, path decoding or wildcard backend.
inline bool parseActivationHttpBackendQuery(
    const std::string& path, std::string& backendId)
{
    static const std::regex canonical(
        R"ACT(^/api/v1/addons/rectools/activation\?backendId=([a-zA-Z0-9_.:-]{1,128})$)ACT");
    std::smatch match;
    if (!std::regex_match(path, match, canonical)) return false;
    backendId = match[1].str();
    return backendId != "." && backendId != "..";
}
} // namespace vdrsuite::addons
