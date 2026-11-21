#pragma once

#include "DashboardController.h"

#include <string>

// Response-only adapter for an ALREADY authorized, activated Device-owned
// Recording session. Does not issue a grant, perform authorization or bypass
// the existing MediaSession service; the caller must stop on failure.
struct PublicRecordingDeviceMediaSessionReady
{
    std::string sessionId;
    std::string backendId;
    std::string publicRecordingId;
    std::string presentationProfileId;
    std::string expiresAt;
    std::string mediaCredential;
    // External video path and cookie Path may differ under nginx proxy rewriting.
    std::string trustedExternalPrefix;
    std::string trustedCookiePrefix;
    int lifetimeSeconds = 0;
    // Authenticated Public-v1 playback projection: no native paths.
    int durationSeconds = 0;
    bool resumeSupported = false;
    bool resumePreparing = false;
    // Only for authenticated Device CREATE, requested by a native player.
    bool nativeMediaAccessRequested = false;
};

class PublicRecordingDeviceMediaSessionResponse final
{
public:
    static ApiResponse afterActivation(
        const PublicRecordingDeviceMediaSessionReady& ready);
};
