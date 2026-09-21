#pragma once

#include "MediaSessionRepository.h"

#include <string>

struct MediaAccessGrantAuthentication
{
    bool authenticated = false;
    std::string reasonCode;
    std::string grantId;
    std::string sessionId;
    std::string routeId;
    long long routeEpoch = 0;
    std::string actorId;
};

class MediaAccessGrantAuthenticator
{
public:
    explicit MediaAccessGrantAuthenticator(
        const MediaSessionRepository& repository,
        int idleTimeoutSeconds = 300,
        int lastSeenWriteIntervalSeconds = 60);

    MediaAccessGrantAuthentication authenticate(
        const std::string& credential,
        const std::string& requestedSessionId) const;

private:
    const MediaSessionRepository& repository_;
    int idleTimeoutSeconds_ = 300;
    int lastSeenWriteIntervalSeconds_ = 60;
};
