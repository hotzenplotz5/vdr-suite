#include "PublicRecordingDeviceMediaSessionResponse.h"

#include <cassert>
#include <string>

int main()
{
    PublicRecordingDeviceMediaSessionReady ready;
    ready.sessionId = "ms_0123456789abcdef0123456789abcdef";
    ready.publicRecordingId = "rec_0123456789abcdef0123456789abcdef";
    ready.backendId = "backend-a";
    ready.presentationProfileId = "hls-fmp4";
    ready.expiresAt = "2026-10-10 20:00:00";
    ready.mediaCredential =
        "mg_0123456789abcdef0123456789abcdef." 
        "AbCdEfGhIjKlMnOpQrStUvWxYz0123456789-_";
    ready.trustedExternalPrefix = "/vdr-suite";
    ready.lifetimeSeconds = 300;

    auto response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 201);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.headers.at("Set-Cookie").find(
        "Path=/vdr-suite/api/v1/media/sessions/" + ready.sessionId + "/") !=
        std::string::npos);
    assert(response.headers.at("Set-Cookie").find(
        "HttpOnly; Secure; SameSite=Strict") != std::string::npos);
    assert(response.body.find(ready.publicRecordingId) != std::string::npos);
    assert(response.body.find(ready.backendId) != std::string::npos);
    assert(response.body.find(
        "/vdr-suite/api/v1/media/sessions/" + ready.sessionId +
        "/hls/master.m3u8") != std::string::npos);
    assert(response.body.find(ready.mediaCredential) == std::string::npos);
    assert(response.body.find("/api/media/sessions") == std::string::npos);
    assert(response.body.find("/srv/vdr") == std::string::npos);

    ready.presentationProfileId = "progressive-direct";
    response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 201);
    assert(response.body.find("/recording/stream.ts") != std::string::npos);
    ready.presentationProfileId = "progressive-fmp4";
    response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 201);
    assert(response.body.find("/recording/stream.mp4") != std::string::npos);
    ready.presentationProfileId = "hls-ts";
    response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 201);
    ready.trustedExternalPrefix = "";
    response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 201);
    assert(response.body.find(
        "\"mediaPath\":\"/api/v1/media/sessions/") != std::string::npos);

    ready.presentationProfileId = "unknown";
    assert(PublicRecordingDeviceMediaSessionResponse::afterActivation(
        ready).statusCode == 503);
    ready.presentationProfileId = "hls-ts";
    ready.publicRecordingId = "../private";
    assert(PublicRecordingDeviceMediaSessionResponse::afterActivation(
        ready).statusCode == 503);
    ready.publicRecordingId = "rec_0123456789abcdef0123456789abcdef";
    ready.trustedExternalPrefix = "/vdr-suite\r\nSet-Cookie: stolen=x";
    response = PublicRecordingDeviceMediaSessionResponse::afterActivation(ready);
    assert(response.statusCode == 503);
    assert(response.headers.find("Set-Cookie") == response.headers.end());
    assert(response.body.find(ready.mediaCredential) == std::string::npos);
    ready.trustedExternalPrefix = "/vdr-suite";
    ready.mediaCredential = "bad; Path=/";
    assert(PublicRecordingDeviceMediaSessionResponse::afterActivation(
        ready).statusCode == 503);
    ready.mediaCredential = "mg_0123456789abcdef0123456789abcdef.AbCdEfGh";
    ready.lifetimeSeconds = 299;
    assert(PublicRecordingDeviceMediaSessionResponse::afterActivation(
        ready).statusCode == 503);
    return 0;
}
