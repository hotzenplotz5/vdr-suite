#pragma once

#include <algorithm>
#include <cctype>
#include <string>

class MediaAccessCredentialHttp
{
public:
    static constexpr const char* CookieName = "vdr_suite_media";
    static constexpr const char* AuthorizationHeader =
        "X-VDR-Suite-Media-Authorization";

    static std::string cookiePath(const std::string& sessionId)
    {
        if (!safeSessionId(sessionId)) return {};
        return "/api/media/sessions/" + sessionId + "/";
    }

    static std::string sessionCookie(
        const std::string& sessionId,
        const std::string& credential,
        int lifetimeSeconds)
    {
        const std::string path = cookiePath(sessionId);
        if (path.empty() ||
            !safeCredential(credential) ||
            lifetimeSeconds < 300 ||
            lifetimeSeconds > 21600)
        {
            return {};
        }

        return std::string(CookieName) + "=" + credential +
            "; Path=" + path +
            "; Max-Age=" + std::to_string(lifetimeSeconds) +
            "; HttpOnly; Secure; SameSite=Strict";
    }

    static std::string expiredSessionCookie(const std::string& sessionId)
    {
        const std::string path = cookiePath(sessionId);
        if (path.empty()) return {};

        return std::string(CookieName) +
            "=; Path=" + path +
            "; Max-Age=0; Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
            "HttpOnly; Secure; SameSite=Strict";
    }

private:
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
