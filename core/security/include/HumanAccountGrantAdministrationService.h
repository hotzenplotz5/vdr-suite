#pragma once

#include "SecurityIdentity.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

class AccountabilityEventRepository;
class Database;
class HumanAccountRepository;
class SecurityIdentityRepository;
class SecurityPermissionGrantRepository;

enum class HumanAccountGrantAdministrationStatus
{
    success,
    invalidRequest,
    accountNotFound,
    accountActorInvalid,
    revisionConflict,
    entropyUnavailable,
    storageError,
};

struct HumanAccountGrantAdministrationContext
{
    std::string actorId;
    ActorType actorType = ActorType::User;
    std::string requestId;
    std::string correlationId;
};

struct HumanAccountGrantResource
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
    HumanAccountGrantResource resource;
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

private:
    Database& database_;
    HumanAccountRepository& accountRepository_;
    SecurityIdentityRepository& identityRepository_;
    SecurityPermissionGrantRepository& grantRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
