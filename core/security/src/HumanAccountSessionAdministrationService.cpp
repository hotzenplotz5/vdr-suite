#include "HumanAccountSessionAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"

#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <ctime>
#include <optional>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16;

bool systemEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0U) return false;
    std::size_t offset = 0U;
    while (offset < size)
    {
        const ssize_t received =
            getrandom(output + offset, size - offset, 0);
        if (received < 0)
        {
            if (errno == EINTR) continue;
            return false;
        }
        if (received == 0) return false;
        offset += static_cast<std::size_t>(received);
    }
    return true;
}

bool safeText(
    const std::string& value,
    std::size_t maximumLength,
    std::size_t minimumLength = 1U)
{
    if (value.size() < minimumLength ||
        value.size() > maximumLength)
    {
        return false;
    }

    return std::none_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character == '\0' ||
                character == '\r' ||
                character == '\n' ||
                std::iscntrl(character);
        });
}

std::string hexEncode(const unsigned char* bytes, std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2U);
    for (std::size_t index = 0U; index < size; ++index)
    {
        result.push_back(Hex[(bytes[index] >> 4U) & 0x0fU]);
        result.push_back(Hex[bytes[index] & 0x0fU]);
    }
    return result;
}

std::optional<std::string> randomId(
    const HumanAccountSessionAdministrationService::EntropySource&
        entropySource)
{
    std::array<unsigned char, IdentifierBytes> bytes{};
    if (!entropySource ||
        !entropySource(bytes.data(), bytes.size()))
    {
        return std::nullopt;
    }
    return "ace_" + hexEncode(bytes.data(), bytes.size());
}

std::string formatTimestamp(
    std::chrono::system_clock::time_point value)
{
    const std::time_t timestamp =
        std::chrono::system_clock::to_time_t(value);
    std::tm utc{};
    if (gmtime_r(&timestamp, &utc) == nullptr)
        return {};

    std::array<char, 32> buffer{};
    if (std::strftime(
            buffer.data(),
            buffer.size(),
            "%Y-%m-%d %H:%M:%S",
            &utc) == 0)
    {
        return {};
    }
    return buffer.data();
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
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const noexcept
    {
        return active_;
    }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
            return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};

HumanAccountSessionAdministrationStatus mapReadStatus(
    HumanAccountCredentialSessionReadStatus status)
{
    switch (status)
    {
        case HumanAccountCredentialSessionReadStatus::success:
            return HumanAccountSessionAdministrationStatus::success;
        case HumanAccountCredentialSessionReadStatus::invalidRequest:
            return HumanAccountSessionAdministrationStatus::invalidRequest;
        case HumanAccountCredentialSessionReadStatus::accountNotFound:
            return HumanAccountSessionAdministrationStatus::accountNotFound;
        case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
            return HumanAccountSessionAdministrationStatus::accountActorInvalid;
        case HumanAccountCredentialSessionReadStatus::sessionNotFound:
            return HumanAccountSessionAdministrationStatus::sessionNotFound;
        case HumanAccountCredentialSessionReadStatus::storageError:
            return HumanAccountSessionAdministrationStatus::storageError;
    }
    return HumanAccountSessionAdministrationStatus::storageError;
}
}

HumanAccountSessionAdministrationService::
HumanAccountSessionAdministrationService(
    Database& database,
    HumanAccountCredentialSessionReadService& readService,
    BrowserSessionLifecycleService& lifecycleService,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      readService_(readService),
      lifecycleService_(lifecycleService),
      accountabilityRepository_(accountabilityRepository),
      entropySource_(
          entropySource
              ? std::move(entropySource)
              : EntropySource(systemEntropy)),
      clock_(
          clock
              ? std::move(clock)
              : Clock([]
                {
                    return std::chrono::system_clock::now();
                }))
{
}

HumanAccountSessionAdministrationResult
HumanAccountSessionAdministrationService::revoke(
    const HumanAccountSessionAdministrationContext& context,
    const std::string& accountId,
    const std::string& sessionId,
    const std::string& expectedResourceRevision)
{
    HumanAccountSessionAdministrationResult result;
    result.accountId = accountId;

    if (!safeText(context.actorId, 128U) ||
        !safeText(context.requestId, 128U) ||
        !safeText(context.correlationId, 128U, 0U) ||
        !safeText(accountId, 128U) ||
        !safeText(sessionId, 128U) ||
        expectedResourceRevision.empty())
    {
        result.status =
            HumanAccountSessionAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_);
    const std::string occurredAt = formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountSessionAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
        return result;

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
        return result;

    const auto auditAndFinish =
        [&](HumanAccountSessionAdministrationStatus status,
            const HumanAccountSessionMetadata& session,
            const std::string& actorId,
            const std::string& decision,
            const std::string& reasonCode,
            const std::string& outcome)
        {
            HumanAccountSessionAdministrationResult finished;
            finished.status = status;
            finished.accountId = accountId;
            finished.actorId = actorId;
            finished.session = session;

            AccountabilityEvent event;
            event.eventId = *eventId;
            event.classes =
                decision == "allow"
                    ? "audit,security,identity"
                    : "audit,security";
            event.eventType =
                decision == "allow"
                    ? "operation.succeeded"
                    : "operation.failed";
            event.severity =
                decision == "allow"
                    ? "info"
                    : "warning";
            event.occurredAt = occurredAt;
            event.actorId = context.actorId;
            event.actorType = "user";
            event.authenticationState = "authenticated";
            event.permission = "accounts.sessions.revoke";
            event.backendId = "*";
            event.operationId =
                "human-account-session:" +
                accountId + ":" + sessionId;
            event.requestId = context.requestId;
            event.correlationId = context.correlationId;
            event.action = "human-account.session.revoke";
            event.decision = decision;
            event.reasonCode = reasonCode;
            event.outcome = outcome;

            if (!accountabilityRepository_.append(event) ||
                !transaction.commit())
            {
                finished.status =
                    HumanAccountSessionAdministrationStatus::storageError;
            }
            return finished;
        };

    const HumanAccountSessionItemReadResult current =
        readService_.readSession(accountId, sessionId);
    const auto mapped = mapReadStatus(current.status);
    if (mapped != HumanAccountSessionAdministrationStatus::success)
    {
        result.status = mapped;
        result.actorId = current.actorId;
        return result;
    }

    result.actorId = current.actorId;
    result.session = current.session;

    const bool terminal =
        current.session.revoked ||
        !current.session.active;

    if (current.session.resourceRevision != expectedResourceRevision)
    {
        if (terminal)
        {
            return auditAndFinish(
                HumanAccountSessionAdministrationStatus::success,
                current.session,
                current.actorId,
                "allow",
                "session_already_terminal",
                "success");
        }

        return auditAndFinish(
            HumanAccountSessionAdministrationStatus::revisionConflict,
            current.session,
            current.actorId,
            "deny",
            "session_revision_conflict",
            "failed");
    }

    if (terminal)
    {
        return auditAndFinish(
            HumanAccountSessionAdministrationStatus::success,
            current.session,
            current.actorId,
            "allow",
            "session_already_terminal",
            "success");
    }

    if (current.session.browserCredentialId.empty() ||
        !lifecycleService_.revokeInActiveTransaction(
            current.session.sessionId,
            current.session.browserCredentialId))
    {
        result.status =
            HumanAccountSessionAdministrationStatus::storageError;
        return result;
    }

    const HumanAccountSessionItemReadResult revoked =
        readService_.readSession(accountId, sessionId);
    if (revoked.status !=
            HumanAccountCredentialSessionReadStatus::success ||
        (!revoked.session.revoked &&
         revoked.session.active) ||
        revoked.session.resourceRevision.empty() ||
        revoked.session.resourceRevision ==
            current.session.resourceRevision)
    {
        result.status =
            HumanAccountSessionAdministrationStatus::storageError;
        return result;
    }

    return auditAndFinish(
        HumanAccountSessionAdministrationStatus::success,
        revoked.session,
        revoked.actorId,
        "allow",
        "session_revoked",
        "success");
}
