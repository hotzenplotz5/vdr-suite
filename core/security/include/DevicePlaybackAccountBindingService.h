#pragma once

#include "SecurityIdentity.h"
#include <cstdint>
#include <string>

class Database;
class SecurityIdentityRepository;
class HumanAccountRepository;
class SecurityPermissionGrantRepository;
class AccountabilityEventRepository;

enum class DevicePlaybackAccountStatus {
    ok, invalid, notFound, revisionConflict, unavailable
};
struct DevicePlaybackAccountBinding {
    std::string deviceId;
    std::string accountId; // empty: intentionally unlinked
    std::int64_t revision = 0;
};
struct DevicePlaybackAccountResult {
    DevicePlaybackAccountStatus status = DevicePlaybackAccountStatus::unavailable;
    DevicePlaybackAccountBinding binding;
};
enum class DevicePlaybackActorStatus {
    deviceOnly, linked, forbidden, unavailable
};
struct DevicePlaybackActorResolution {
    DevicePlaybackActorStatus status = DevicePlaybackActorStatus::unavailable;
    std::string actorId;
    std::string accountId;
};

// One explicit, administrator-assigned account per Device. The Device's
// Service identity, media grant, MediaSession owner and credentials are never
// converted to a User; only the scoped ContinueWatching DATA owner can use
// the account's already-existing Actor after verifying BOTH current grants.
class DevicePlaybackAccountBindingService final {
public:
    DevicePlaybackAccountBindingService(
        Database& database, SecurityIdentityRepository& identities,
        HumanAccountRepository& accounts,
        SecurityPermissionGrantRepository& grants,
        AccountabilityEventRepository& audit);

    bool ensureSchema();

    // Revision zero represents a never-linked Device. A zero-account update
    // creates a durable unlink tombstone, preventing stale clients from
    // silently recreating an older assignment.
    DevicePlaybackAccountResult read(const std::string& deviceId) const;
    DevicePlaybackAccountResult set(
        const std::string& administratorActorId,
        const std::string& deviceId,
        const std::string& accountId, // empty explicitly unlinks
        std::int64_t expectedRevision,
        const std::string& requestId);

    DevicePlaybackActorResolution resolve(
        const RequestSecurityContext& authenticatedDevice,
        const std::string& backendId) const;

private:
    DevicePlaybackAccountResult readInActiveTransaction(
        const std::string& deviceId) const;

    Database& database_;
    SecurityIdentityRepository& identities_;
    HumanAccountRepository& accounts_;
    SecurityPermissionGrantRepository& grants_;
    AccountabilityEventRepository& audit_;
};
