#include "PublicRecordingDeviceMediaSessionResponse.h"

#include "MediaAccessCredentialHttp.h"

#include <algorithm>
#include <cctype>
#include <string>

namespace
{
bool identifier(const std::string& value, const std::string& prefix)
{
    return value.size() == prefix.size() + 32U &&
        value.compare(0, prefix.size(), prefix) == 0 &&
        std::all_of(
            value.begin() + static_cast<std::string::difference_type>(prefix.size()),
            value.end(),
            [](unsigned char c) {
                return (c >= '0' && c <= '9') ||
                    (c >= 'a' && c <= 'f');
            });
}

bool backend(const std::string& value)
{
    return !value.empty() && value.size() <= 128U &&
        value != "." && value != ".." && value != "*" &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '_' || c == '-' || c == '.';
        });
}

bool expiry(const std::string& value)
{
    if (value.size() != 19U) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 4U || i == 7U) {
            if (value[i] != '-') return false;
        } else if (i == 10U) {
            if (value[i] != ' ') return false;
        } else if (i == 13U || i == 16U) {
            if (value[i] != ':') return false;
        } else if (!std::isdigit(static_cast<unsigned char>(value[i]))) {
            return false;
        }
    }
    return true;
}

const char* suffix(const std::string& profile)
{
    if (profile == "hls-fmp4" || profile == "hls-ts")
        return "/hls/master.m3u8";
    if (profile == "progressive-direct")
        return "/recording/stream.ts";
    if (profile == "progressive-fmp4")
        return "/recording/stream.mp4";
    return nullptr;
}

ApiResponse failure()
{
    ApiResponse result;
    result.statusCode = 503;
    result.contentType = "application/json";
    result.headers["Cache-Control"] = "no-store";
    result.headers["X-Content-Type-Options"] = "nosniff";
    result.body = "{\"error\":{\"code\":\"public_media_session_response_unavailable\"}}";
    return result;
}
} // namespace

ApiResponse PublicRecordingDeviceMediaSessionResponse::afterActivation(
    const PublicRecordingDeviceMediaSessionReady& ready)
{
    // Do not allow a native VDR identity, guessed profile, unsafe path or
    // untrusted expiry to become part of a public response.
    if (!identifier(ready.sessionId, "ms_") ||
        !identifier(ready.publicRecordingId, "rec_") ||
        !backend(ready.backendId) ||
        !expiry(ready.expiresAt) ||
        suffix(ready.presentationProfileId) == nullptr)
        return failure();

    const std::string cookie = MediaAccessCredentialHttp::publicV1SessionCookie(
        ready.sessionId,
        ready.mediaCredential,
        ready.lifetimeSeconds,
        ready.trustedExternalPrefix);
    if (cookie.empty()) return failure();

    ApiResponse result;
    result.statusCode = 201;
    result.contentType = "application/json";
    result.headers["Cache-Control"] = "no-store";
    result.headers["X-Content-Type-Options"] = "nosniff";
    result.headers["Set-Cookie"] = cookie;
    const std::string path = ready.trustedExternalPrefix +
        "/api/v1/media/sessions/" + ready.sessionId +
        suffix(ready.presentationProfileId);
    result.body = "{\"mediaSession\":{\"id\":\"" +
        ready.sessionId + "\",\"state\":\"ready\",\"backendId\":\"" +
        ready.backendId + "\",\"recordingId\":\"" +
        ready.publicRecordingId + "\",\"presentationProfileId\":\"" +
        ready.presentationProfileId + "\",\"mediaPath\":\"" +
        path + "\",\"expiresAt\":\"" + ready.expiresAt + "\"}}";
    return result;
}
