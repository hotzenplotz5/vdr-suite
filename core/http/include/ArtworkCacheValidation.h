#pragma once

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "PublicResourcePreconditions.h"
#include <openssl/sha.h>
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace vdrsuite::http
{
inline std::string artworkDigestTag(const std::string& bytes)
{
    unsigned char digest[SHA256_DIGEST_LENGTH];
    if (!SHA256(reinterpret_cast<const unsigned char*>(bytes.data()), bytes.size(), digest)) return {};
    return publicStrongEntityTag(std::string(reinterpret_cast<const char*>(digest), sizeof(digest)));
}

inline void appendArtworkField(std::string& out, const std::string& field)
{
    out += std::to_string(field.size()) + ':' + field;
}

// Stable, non-credential partition of the existing authenticated actor and grants.
inline std::string artworkCacheScope(const std::string& actor, const std::string& backend,
                                     std::vector<std::string> grants)
{
    if (actor.empty() || backend.empty() || backend == "*") return {};
    std::sort(grants.begin(), grants.end());
    std::string material = "vdr-suite-artwork-v1";
    appendArtworkField(material, actor);
    appendArtworkField(material, backend);
    for (const auto& grant : grants) appendArtworkField(material, grant);
    const auto tag = artworkDigestTag(material);
    return tag.empty() ? std::string{} : tag.substr(1, tag.size() - 2);
}

inline void revalidateScopedArtwork(const HttpServerRequest& request, HttpServerResponse& response)
{
    const auto partition = response.headers.find("X-VDR-Suite-Artwork-Scope");
    if (partition == response.headers.end()) return;
    response.headers["Cache-Control"] = "private, no-cache";
    response.headers["Vary"] = "Cookie, Authorization";
    const auto type = response.headers.find("Content-Type");
    if (response.statusCode != 200 || type == response.headers.end() ||
        (type->second != "image/jpeg" && type->second != "image/png" && type->second != "image/webp") ||
        response.body.empty() || response.body.size() > 256U * 1024U) {
        response.headers.erase(partition);
        return;
    }
    std::string bytes;
    appendArtworkField(bytes, partition->second);
    appendArtworkField(bytes, request.path);
    appendArtworkField(bytes, type->second);
    bytes += response.body;
    const std::string tag = artworkDigestTag(bytes);
    if (tag.empty()) { response.headers.erase(partition); return; }
    response.headers["ETag"] = tag;
    response.headers["X-VDR-Suite-Artwork-Bytes"] = std::to_string(response.body.size());
    response.headers.erase("Last-Modified");
    std::string condition;
    for (const auto& header : request.headers) {
        std::string name = header.first;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
        if (name == "if-none-match") { condition = header.second; break; }
    }
    if (publicEvaluateIfNoneMatch(condition, tag) == PublicEntityTagConditionResult::matched) {
        response.statusCode = 304;
        response.body.clear();
        response.headers.erase("Content-Length");
    }
}
}
