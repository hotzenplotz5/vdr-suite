#pragma once

#include "SecurityIdentity.h"

#include <string>
#include <vector>

class Database;
class SecurityIdentityRepository;
class DeviceCredentialVerifierRepository;
class SecurityPermissionGrantRepository;
class AccountabilityEventRepository;

// MU.10E1: device-actor grants only. Never create actors, inherit approver
// permissions, or allow a Service Actor to acquire an administrator role.
enum class DeviceGrantAdministrationStatus
{
    ok, invalid, notFound, revisionConflict, unavailable
};

struct DeviceGrantSet
{
    std::string deviceId;
    std::string actorId;
    std::vector<PermissionGrant> grants;
    std::string revision;
};

struct DeviceGrantAdministrationResult
{
    DeviceGrantAdministrationStatus status =
        DeviceGrantAdministrationStatus::unavailable;
    DeviceGrantSet grantSet;
};

struct DeviceGrantAdministrationContext
{
    std::string administratorActorId;
    std::string requestId;
    std::string correlationId;
};

class DeviceGrantAdministrationService
{
public:
    DeviceGrantAdministrationService(
        Database& database,
        SecurityIdentityRepository& identities,
        DeviceCredentialVerifierRepository& verifiers,
        SecurityPermissionGrantRepository& grants,
        AccountabilityEventRepository& audit);

    DeviceGrantAdministrationResult read(
        const std::string& deviceId) const;

    DeviceGrantAdministrationResult setGrant(
        const DeviceGrantAdministrationContext& context,
        const std::string& deviceId,
        const std::string& expectedRevision,
        const std::string& permission,
        const std::string& backendId,
        bool active);

    static bool supportedGrant(
        const std::string& permission, const std::string& backendId);

private:
    DeviceGrantAdministrationResult readInActiveTransaction(
        const std::string& deviceId) const;

    Database& database_;
    SecurityIdentityRepository& identities_;
    DeviceCredentialVerifierRepository& verifiers_;
    SecurityPermissionGrantRepository& grants_;
    AccountabilityEventRepository& audit_;
};
