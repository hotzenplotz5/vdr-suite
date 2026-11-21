#pragma once

#include "MediaSessionRepository.h"

#include <chrono>
#include <functional>
#include <string>

struct MediaSessionIssuanceRequest
{
    std::string actorId;
    std::string backendId;
    std::string resourceKind;
    std::string resourceId;
    std::string presentationProfileId;
    std::string providerId;
    int lifetimeSeconds = 21600;
};

struct IssuedMediaSession
{
    std::string sessionId;
    std::string routeId;
    long long routeEpoch = 0;
    std::string leaseId;
    std::string workspaceId;
    std::string grantId;
    std::string accessCredential;
    std::string expiresAt;

    IssuedMediaSession() = default;
    ~IssuedMediaSession();
    IssuedMediaSession(const IssuedMediaSession&) = delete;
    IssuedMediaSession& operator=(const IssuedMediaSession&) = delete;
    IssuedMediaSession(IssuedMediaSession&& other) noexcept;
    IssuedMediaSession& operator=(IssuedMediaSession&& other) noexcept;

    void clearSecret() noexcept;
};

struct MediaSessionIssuanceResult
{
    bool issued = false;
    std::string reasonCode;
    IssuedMediaSession session;
};

class MediaSessionIssuanceService
{
public:
    using EntropySource = std::function<bool(unsigned char*, std::size_t)>;
    using Clock = std::function<std::chrono::system_clock::time_point()>;

    explicit MediaSessionIssuanceService(
        MediaSessionRepository& repository,
        EntropySource entropySource = {},
        Clock clock = {});

    MediaSessionIssuanceResult issue(
        const MediaSessionIssuanceRequest& request);

private:
    MediaSessionRepository& repository_;
    EntropySource entropySource_;
    Clock clock_;
};
