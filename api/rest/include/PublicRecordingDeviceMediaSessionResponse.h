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
    std::string trustedExternalPrefix;
    int lifetimeSeconds = 0;
};

class PublicRecordingDeviceMediaSessionResponse final
{
public:
    static ApiResponse afterActivation(
        const PublicRecordingDeviceMediaSessionReady& ready);
};
