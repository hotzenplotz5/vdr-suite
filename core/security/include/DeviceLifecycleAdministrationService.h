#pragma once

#include <string>

class Database;
class SecurityIdentityRepository;
class DeviceCredentialVerifierRepository;
class AccountabilityEventRepository;

enum class DeviceLifecycleTarget { Device, Credential };
enum class DeviceLifecycleStatus
{
    ok, invalid, notFound, revisionConflict, unavailable
};

struct DeviceLifecycleResource
{
    std::string deviceId;
    std::string actorId;
    std::string credentialId;
    bool active = false;
    bool revoked = false;
    std::string resourceRevision;
};

struct DeviceLifecycleResult
{
    DeviceLifecycleStatus status = DeviceLifecycleStatus::unavailable;
    DeviceLifecycleResource resource;
};

struct DeviceLifecycleContext
{
    std::string administratorActorId;
    std::string requestId;
    std::string correlationId;
};

// MU.10E2: revoke canonical device or device-app credential under one audited
// transaction. Never rotate, mint, re-pair, or delete identity/grant history.
class DeviceLifecycleAdministrationService
{
public:
    DeviceLifecycleAdministrationService(
        Database& database,
        SecurityIdentityRepository& identities,
        DeviceCredentialVerifierRepository& verifiers,
        AccountabilityEventRepository& accountability);

    DeviceLifecycleResult read(
        DeviceLifecycleTarget target, const std::string& deviceId,
        const std::string& credentialId = "") const;

    DeviceLifecycleResult revoke(
        const DeviceLifecycleContext& context,
        DeviceLifecycleTarget target, const std::string& deviceId,
        const std::string& credentialId,
        const std::string& expectedRevision);

private:
    DeviceLifecycleResult readInTransaction(
        DeviceLifecycleTarget target, const std::string& deviceId,
        const std::string& credentialId) const;

    Database& database_;
    SecurityIdentityRepository& identities_;
    DeviceCredentialVerifierRepository& verifiers_;
    AccountabilityEventRepository& accountability_;
};
