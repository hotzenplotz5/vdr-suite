#pragma once

#include "SecurityIdentity.h"

#include <chrono>
#include <functional>
#include <string>
#include <vector>

class AccountabilityEventRepository;
class Database;
class HumanAccountAdministrationRepository;
class HumanAccountRepository;
class SecurityIdentityRepository;
class SecurityPermissionGrantRepository;

enum class HumanAccountGrantAdministrationStatus
{
    success,
    accountNotFound,
    accountActorInvalid,
    invalidGrant,
    revisionConflict,
    finalAdministrator,
    entropyUnavailable,
    storageError,
};

struct HumanAccountGrantAdministrationContext
{
    std::string actorId;
    std::string requestId;
    std::string correlationId;
};

struct HumanAccountGrantSet
{
    std::string accountId;
    std::string actorId;
    std::vector<PermissionGrant> grants;
    std::string resourceRevision;
};

struct HumanAccountGrantAdministrationResult
{
    HumanAccountGrantAdministrationStatus status =
        HumanAccountGrantAdministrationStatus::storageError;
    HumanAccountGrantSet grantSet;
};

class HumanAccountGrantAdministrationService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountGrantAdministrationService(
        Database& database,
        HumanAccountRepository& accountRepository,
        SecurityIdentityRepository& identityRepository,
        SecurityPermissionGrantRepository& grantRepository,
        HumanAccountAdministrationRepository& administrationRepository,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountGrantAdministrationResult read(
        const std::string& accountId) const;

    HumanAccountGrantAdministrationResult setGrant(
        const HumanAccountGrantAdministrationContext& context,
        const std::string& accountId,
        const std::string& expectedResourceRevision,
        const std::string& permission,
        const std::string& backendId,
        bool active);

    static bool supportedGrant(
        const std::string& permission,
        const std::string& backendId);

    static std::vector<std::string> supportedGrantPermissions();
    static std::vector<std::string> supportedGrantScopeKinds();

private:
    HumanAccountGrantAdministrationResult readInActiveTransaction(
        const std::string& accountId) const;

    Database& database_;
    HumanAccountRepository& accountRepository_;
    SecurityIdentityRepository& identityRepository_;
    SecurityPermissionGrantRepository& grantRepository_;
    HumanAccountAdministrationRepository& administrationRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
