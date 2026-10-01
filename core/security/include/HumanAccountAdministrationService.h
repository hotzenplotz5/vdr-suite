#pragma once

#include "HumanAccountRepository.h"
#include "SecurityIdentity.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionLifecycleService;
class Database;
class SecurityIdentityRepository;

enum class HumanAccountAdministrationStatus
{
    success,
    invalidRequest,
    accountNotFound,
    revisionConflict,
    accountActorInvalid,
    finalAdministrator,
    entropyUnavailable,
    storageError,
};

struct HumanAccountAdministrationContext
{
    std::string actorId;
    ActorType actorType = ActorType::User;
    std::string requestId;
    std::string correlationId;
};

struct HumanAccountAdministrationResult
{
    HumanAccountAdministrationStatus status =
        HumanAccountAdministrationStatus::storageError;
    HumanAccountRecord account;
    std::size_t revokedBrowserSessions = 0;
};

class HumanAccountAdministrationService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountAdministrationService(
        Database& database,
        HumanAccountRepository& accountRepository,
        HumanAccountAdministrationRepository& administrationRepository,
        SecurityIdentityRepository& identityRepository,
        BrowserSessionLifecycleService& browserSessionLifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountAdministrationResult modifyDisplayName(
        const HumanAccountAdministrationContext& context,
        const std::string& accountId,
        std::uint64_t expectedRevision,
        const std::string& displayName);

    HumanAccountAdministrationResult setActive(
        const HumanAccountAdministrationContext& context,
        const std::string& accountId,
        std::uint64_t expectedRevision,
        bool active);

private:
    Database& database_;
    HumanAccountRepository& accountRepository_;
    HumanAccountAdministrationRepository& administrationRepository_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionLifecycleService& browserSessionLifecycleService_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
