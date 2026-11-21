#pragma once

#include "HumanAccountCredentialSessionReadService.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionCredentialRepository;
class BrowserSessionLifecycleService;
class Database;
class HumanAccountAdministrationRepository;
class SecurityIdentityRepository;

enum class HumanAccountCredentialAdministrationStatus
{
    success,
    invalidRequest,
    accountNotFound,
    accountActorInvalid,
    credentialNotFound,
    credentialInvalid,
    revisionConflict,
    finalAdministrator,
    entropyUnavailable,
    storageError,
};

struct HumanAccountCredentialAdministrationContext
{
    std::string actorId;
    std::string requestId;
    std::string correlationId;
};

struct HumanAccountCredentialAdministrationResult
{
    HumanAccountCredentialAdministrationStatus status =
        HumanAccountCredentialAdministrationStatus::storageError;
    std::string accountId;
    std::string actorId;
    HumanAccountCredentialMetadata credential;
    std::size_t revokedBrowserSessions = 0U;
};

class HumanAccountCredentialAdministrationService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountCredentialAdministrationService(
        Database& database,
        HumanAccountCredentialSessionReadService& readService,
        HumanAccountAdministrationRepository& administrationRepository,
        SecurityIdentityRepository& identityRepository,
        BrowserSessionCredentialRepository& browserSessionRepository,
        BrowserSessionLifecycleService& browserSessionLifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountCredentialAdministrationResult revoke(
        const HumanAccountCredentialAdministrationContext& context,
        const std::string& accountId,
        const std::string& credentialId,
        const std::string& expectedResourceRevision);

private:
    Database& database_;
    HumanAccountCredentialSessionReadService& readService_;
    HumanAccountAdministrationRepository& administrationRepository_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionCredentialRepository& browserSessionRepository_;
    BrowserSessionLifecycleService& browserSessionLifecycleService_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
