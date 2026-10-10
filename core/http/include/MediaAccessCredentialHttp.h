#pragma once

#include <algorithm>
#include <cctype>
#include <string>

class MediaAccessCredentialHttp
{
public:
    inline static constexpr char CookieName[] = "vdr_suite_media";
    inline static constexpr char AuthorizationHeader[] =
        "X-VDR-Suite-Media-Authorization";

    static std::string cookiePath(const std::string& sessionId)
    {
        if (!safeSessionId(sessionId)) return {};
        return "/api/media/sessions/" + sessionId + "/";
    }

    // The trusted external URL prefix is deployment configuration, never a
    // request-supplied Host, Origin or X-Forwarded-Prefix header. Empty means
    // the reverse proxy exposes /api/v1 at the origin root.
    static std::string publicV1CookiePath(
        const std::string& sessionId,
        const std::string& trustedExternalPrefix)
    {
        if (!safeSessionId(sessionId) ||
            !safeExternalPrefix(trustedExternalPrefix))
            return {};
        return trustedExternalPrefix +
            "/api/v1/media/sessions/" + sessionId + "/";
    }

    static std::string sessionCookie(
        const std::string& sessionId,
        const std::string& credential,
        int lifetimeSeconds)
    {
        return makeCookie(cookiePath(sessionId), credential, lifetimeSeconds);
    }

    static std::string expiredSessionCookie(const std::string& sessionId)
    {
        return makeExpiredCookie(cookiePath(sessionId));
    }

    // Explicitly opt in when a versioned Device MediaSession issuer is
    // available. The legacy browser issuer must retain its old cookie path.
    static std::string publicV1SessionCookie(
        const std::string& sessionId,
        const std::string& credential,
        int lifetimeSeconds,
        const std::string& trustedExternalPrefix)
    {
        return makeCookie(
            publicV1CookiePath(sessionId, trustedExternalPrefix),
            credential,
            lifetimeSeconds);
    }

    static std::string expiredPublicV1SessionCookie(
        const std::string& sessionId,
        const std::string& trustedExternalPrefix)
    {
        return makeExpiredCookie(
            publicV1CookiePath(sessionId, trustedExternalPrefix));
    }

private:
    static std::string makeCookie(
        const std::string& path,
        const std::string& credential,
        int lifetimeSeconds)
    {
        if (path.empty() ||
            !safeCredential(credential) ||
            lifetimeSeconds < 300 ||
            lifetimeSeconds > 21600)
            return {};

        return std::string(CookieName) + "=" + credential +
            "; Path=" + path +
            "; Max-Age=" + std::to_string(lifetimeSeconds) +
            "; HttpOnly; Secure; SameSite=Strict";
    }

    static std::string makeExpiredCookie(const std::string& path)
    {
        if (path.empty()) return {};

        return std::string(CookieName) +
            "=; Path=" + path +
            "; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
            "HttpOnly; Secure; SameSite=Strict";
    }

    static bool safeExternalPrefix(const std::string& prefix)
    {
        if (prefix.empty()) return true;
        if (prefix.size() > 128 || prefix.front() != '/' ||
            prefix.back() == '/')
            return false;

        std::size_t start = 1;
        while (start < prefix.size()) {
            const std::size_t end = prefix.find('/', start);
            const std::string segment = prefix.substr(start, end - start);
            if (segment.empty() || segment == "." || segment == ".." ||
                !std::all_of(
                    segment.begin(), segment.end(),
                    [](unsigned char character) {
                        return std::isalnum(character) ||
                            character == '-' || character == '_';
                    }))
                return false;
            if (end == std::string::npos) break;
            start = end + 1;
        }
        return true;
    }

    static bool safeSessionId(const std::string& value)
    {
        if (value.empty() || value.size() > 128) return false;
        return std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_' ||
                    character == '.' ||
                    character == ':';
            });
    }

    static bool safeCredential(const std::string& value)
    {
        if (value.empty() || value.size() > 512) return false;
        return std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '-' ||
                    character == '_' ||
                    character == '.';
            });
    }
};
