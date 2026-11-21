#include "BrowserSessionRetentionService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

namespace
{
constexpr const char* CleanupAction = "browser.session.cleanup";
constexpr const char* CleanupReason = "browser_session_retention_elapsed";

std::string nowUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    if (gmtime_r(&now, &utc) == nullptr)
    {
        return {};
    }

    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_)
        {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
        {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};
}

BrowserSessionRetentionService::BrowserSessionRetentionService(
    Database& database,
    BrowserSessionCredentialRepository& credentialRepository,
    SecurityIdentityRepository& identityRepository,
    AccountabilityEventRepository& accountabilityRepository)
    : database_(database),
      credentialRepository_(credentialRepository),
      identityRepository_(identityRepository),
      accountabilityRepository_(accountabilityRepository)
{
}

bool BrowserSessionRetentionService::cleanup(
    const BrowserSessionRetentionConfiguration& retentionConfiguration,
    const BrowserSessionIdleConfiguration& idleConfiguration)
{
    if (!retentionConfiguration.valid())
    {
        return false;
    }
    if (!retentionConfiguration.enabled())
    {
        return true;
    }
    if (!idleConfiguration.valid())
    {
        return false;
    }

    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        return false;
    }

    const auto candidates =
        credentialRepository_.findTerminalRetentionCandidates(
            retentionConfiguration.seconds,
            idleConfiguration.timeoutSeconds,
            BrowserSessionRetentionConfiguration::BatchSize);
    if (!candidates.has_value())
    {
        return false;
    }

    for (const TerminalBrowserSessionCandidate& candidate : *candidates)
    {
        const auto remainsCandidate =
            credentialRepository_.remainsTerminalRetentionCandidate(
                candidate,
                retentionConfiguration.seconds,
                idleConfiguration.timeoutSeconds);
        if (!remainsCandidate.has_value() || !*remainsCandidate)
        {
            return false;
        }

        if (!appendCleanupEvent(candidate) ||
            !credentialRepository_.deleteTerminalRetentionCandidate(candidate) ||
            !identityRepository_.deleteSessionIfUnreferenced(
                candidate.sessionId,
                candidate.actorId,
                candidate.deviceId) ||
            !identityRepository_
                 .deleteBrowserSessionCredentialIfUnreferenced(
                     candidate.credentialId,
                     candidate.actorId))
        {
            return false;
        }
    }

    return transaction.commit();
}

bool BrowserSessionRetentionService::appendCleanupEvent(
    const TerminalBrowserSessionCandidate& candidate)
{
    AccountabilityEvent event;
    event.eventId = opaqueId("cleanup");
    event.classes = "security,lifecycle,maintenance";
    event.eventType = "operation.succeeded";
    event.severity = "info";
    event.occurredAt = nowUtc();
    event.actorId = candidate.actorId;
    event.actorType = "system";
    event.deviceId = candidate.deviceId;
    event.sessionId = candidate.sessionId;
    event.authenticationState = "system-maintenance";
    event.permission = "";
    event.backendId = "*";
    event.operationId = event.eventId;
    event.requestId = event.eventId;
    event.correlationId = "";
    event.action = CleanupAction;
    event.decision = "completed";
    event.reasonCode = CleanupReason;
    event.outcome = "deleted";
    return accountabilityRepository_.append(event);
}

std::string BrowserSessionRetentionService::opaqueId(
    const std::string& prefix) const
{
    const auto ticks =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const unsigned long long sequence =
        idCounter_.fetch_add(1) + 1;
    std::ostringstream output;
    output << prefix << '-' << std::hex
           << static_cast<unsigned long long>(ticks)
           << '-' << sequence;
    return output.str();
}