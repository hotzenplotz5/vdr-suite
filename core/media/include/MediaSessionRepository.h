#pragma once

#include <optional>
#include <string>
#include <vector>

class Database;

struct MediaSessionBundleRegistration
{
    std::string sessionId;
    std::string actorId;
    std::string backendId;
    std::string resourceKind;
    std::string resourceId;
    std::string presentationProfileId;
    std::string routeId;
    long long routeEpoch = 1;
    std::string providerId;
    std::string leaseId;
    std::string workspaceId;
    std::string grantId;
    std::string secretHash;
    std::string expiresAt;
};

struct StoredMediaSession
{
    std::string sessionId;
    std::string actorId;
    std::string backendId;
    std::string resourceKind;
    std::string resourceId;
    std::string state;
    std::string presentationProfileId;
    std::string terminalReason;
};

struct StoredMediaAccessGrant
{
    std::string grantId;
    std::string sessionId;
    std::string routeId;
    long long routeEpoch = 0;
    std::string actorId;
    std::string secretHash;
    std::string expiresAt;
    std::string lastSeenAt;
    bool active = false;
    bool expired = false;
    bool idleExpired = false;
    bool revoked = false;
};

class MediaSessionRepository
{
public:
    explicit MediaSessionRepository(Database& database);

    bool ensureSchema();

    bool insertProvisioningBundle(
        const MediaSessionBundleRegistration& registration);

    bool activateBundle(const std::string& sessionId);

    bool failBundle(
        const std::string& sessionId,
        const std::string& reasonCode);

    bool endBundle(
        const std::string& sessionId,
        const std::string& reasonCode);

    std::optional<StoredMediaSession> findSession(
        const std::string& sessionId) const;

    std::optional<StoredMediaAccessGrant> findResolvedGrant(
        const std::string& grantId,
        int idleTimeoutSeconds) const;

    std::optional<bool> touchGrantIfDue(
        const std::string& grantId,
        int minimumIntervalSeconds) const;

    bool recoverNonTerminalBundles();

    static bool supportsSecretHash(const std::string& secretHash);

private:
    Database& database_;
};
