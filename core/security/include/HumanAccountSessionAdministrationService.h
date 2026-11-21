#pragma once

#include "HumanAccountCredentialSessionReadService.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionLifecycleService;
class Database;

enum class HumanAccountSessionAdministrationStatus
{
    success,
    invalidRequest,
    accountNotFound,
    accountActorInvalid,
    sessionNotFound,
    revisionConflict,
    entropyUnavailable,
    storageError,
};

struct HumanAccountSessionAdministrationContext
{
    std::string actorId;
    std::string requestId;
    std::string correlationId;
};

struct HumanAccountSessionAdministrationResult
{
    HumanAccountSessionAdministrationStatus status =
        HumanAccountSessionAdministrationStatus::storageError;
    std::string accountId;
    std::string actorId;
    HumanAccountSessionMetadata session;
};

class HumanAccountSessionAdministrationService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountSessionAdministrationService(
        Database& database,
        HumanAccountCredentialSessionReadService& readService,
        BrowserSessionLifecycleService& lifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountSessionAdministrationResult revoke(
        const HumanAccountSessionAdministrationContext& context,
        const std::string& accountId,
        const std::string& sessionId,
        const std::string& expectedResourceRevision);

private:
    Database& database_;
    HumanAccountCredentialSessionReadService& readService_;
    BrowserSessionLifecycleService& lifecycleService_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
