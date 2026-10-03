#include "HumanAccountCredentialAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "HumanAccountAdministrationRepository.h"
#include "SecurityIdentityRepository.h"

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
constexpr std::size_t IdentifierBytes = 16U;

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
    const HumanAccountCredentialAdministrationService::EntropySource&
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

HumanAccountCredentialAdministrationStatus mapReadStatus(
    HumanAccountCredentialSessionReadStatus status)
{
    switch (status)
    {
        case HumanAccountCredentialSessionReadStatus::success:
            return HumanAccountCredentialAdministrationStatus::success;
        case HumanAccountCredentialSessionReadStatus::invalidRequest:
            return HumanAccountCredentialAdministrationStatus::invalidRequest;
        case HumanAccountCredentialSessionReadStatus::accountNotFound:
            return HumanAccountCredentialAdministrationStatus::accountNotFound;
        case HumanAccountCredentialSessionReadStatus::accountActorInvalid:
            return HumanAccountCredentialAdministrationStatus::accountActorInvalid;
        case HumanAccountCredentialSessionReadStatus::credentialNotFound:
            return HumanAccountCredentialAdministrationStatus::credentialNotFound;
        case HumanAccountCredentialSessionReadStatus::sessionNotFound:
        case HumanAccountCredentialSessionReadStatus::storageError:
            return HumanAccountCredentialAdministrationStatus::storageError;
    }
    return HumanAccountCredentialAdministrationStatus::storageError;
}
}

HumanAccountCredentialAdministrationService::
HumanAccountCredentialAdministrationService(
    Database& database,
    HumanAccountCredentialSessionReadService& readService,
    HumanAccountAdministrationRepository& administrationRepository,
    SecurityIdentityRepository& identityRepository,
    BrowserSessionCredentialRepository& browserSessionRepository,
    BrowserSessionLifecycleService& browserSessionLifecycleService,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      readService_(readService),
      administrationRepository_(administrationRepository),
      identityRepository_(identityRepository),
      browserSessionRepository_(browserSessionRepository),
      browserSessionLifecycleService_(browserSessionLifecycleService),
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

HumanAccountCredentialAdministrationResult
HumanAccountCredentialAdministrationService::revoke(
    const HumanAccountCredentialAdministrationContext& context,
    const std::string& accountId,
    const std::string& credentialId,
    const std::string& expectedResourceRevision)
{
    HumanAccountCredentialAdministrationResult result;
    result.accountId = accountId;

    if (!safeText(context.actorId, 128U) ||
        !safeText(context.requestId, 128U) ||
        !safeText(context.correlationId, 128U, 0U) ||
        !safeText(accountId, 128U) ||
        !safeText(credentialId, 128U) ||
        expectedResourceRevision.empty())
    {
        result.status =
            HumanAccountCredentialAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_);
    const std::string occurredAt = formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountCredentialAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
        return result;

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
        return result;

    const auto auditAndFinish =
        [&](HumanAccountCredentialAdministrationStatus status,
            const HumanAccountCredentialMetadata& credential,
            const std::string& actorId,
            std::size_t revokedBrowserSessions,
            const std::string& decision,
            const std::string& reasonCode,
            const std::string& outcome)
        {
            HumanAccountCredentialAdministrationResult finished;
            finished.status = status;
            finished.accountId = accountId;
            finished.actorId = actorId;
            finished.credential = credential;
            finished.revokedBrowserSessions = revokedBrowserSessions;

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
            event.permission = "accounts.credentials.revoke";
            event.backendId = "*";
            event.operationId =
                "human-account-credential:" +
                accountId + ":" + credentialId;
            event.requestId = context.requestId;
            event.correlationId = context.correlationId;
            event.action = "human-account.credential.revoke";
            event.decision = decision;
            event.reasonCode = reasonCode;
            event.outcome = outcome;

            if (!accountabilityRepository_.append(event) ||
                !transaction.commit())
            {
                finished.status =
                    HumanAccountCredentialAdministrationStatus::storageError;
            }
            return finished;
        };

    const HumanAccountCredentialItemReadResult current =
        readService_.readCredential(accountId, credentialId);
    const auto mapped = mapReadStatus(current.status);
    if (mapped != HumanAccountCredentialAdministrationStatus::success)
    {
        result.status = mapped;
        result.actorId = current.actorId;
        return result;
    }

    result.actorId = current.actorId;
    result.credential = current.credential;

    if (current.credential.credentialType != "human-password")
    {
        return auditAndFinish(
            HumanAccountCredentialAdministrationStatus::credentialInvalid,
            current.credential,
            current.actorId,
            0U,
            "deny",
            "credential_type_not_revocable",
            "failed");
    }

    const bool terminal =
        current.credential.revoked ||
        !current.credential.active;

    if (current.credential.resourceRevision !=
            expectedResourceRevision &&
        !terminal)
    {
        return auditAndFinish(
            HumanAccountCredentialAdministrationStatus::revisionConflict,
            current.credential,
            current.actorId,
            0U,
            "deny",
            "credential_revision_conflict",
            "failed");
    }

    if (!terminal)
    {
        const auto finalAdministrator =
            administrationRepository_.
                wouldRevokeFinalUsableAdministrator(
                    credentialId);
        if (!finalAdministrator.has_value())
            return result;

        if (*finalAdministrator)
        {
            return auditAndFinish(
                HumanAccountCredentialAdministrationStatus::finalAdministrator,
                current.credential,
                current.actorId,
                0U,
                "deny",
                "final_usable_administrator",
                "failed");
        }
    }

    const auto browserSessions =
        browserSessionRepository_.
            findByIssuedFromCredentialId(credentialId);
    if (!browserSessions.has_value())
        return result;

    if (!current.credential.revoked &&
        !identityRepository_.revokeCredential(credentialId))
    {
        return result;
    }

    std::size_t revokedBrowserSessions = 0U;
    for (const StoredBrowserSessionCredential& browserSession :
         *browserSessions)
    {
        if (browserSession.actorId != current.actorId ||
            browserSession.issuedFromCredentialId != credentialId ||
            !browserSessionLifecycleService_.revokeInActiveTransaction(
                browserSession.sessionId,
                browserSession.credentialId))
        {
            return result;
        }
        ++revokedBrowserSessions;
    }

    const HumanAccountCredentialItemReadResult revoked =
        readService_.readCredential(accountId, credentialId);
    if (revoked.status !=
            HumanAccountCredentialSessionReadStatus::success ||
        revoked.credential.active ||
        !revoked.credential.revoked ||
        revoked.credential.resourceRevision.empty())
    {
        return result;
    }

    if (!terminal &&
        revoked.credential.resourceRevision ==
            current.credential.resourceRevision)
    {
        return result;
    }

    return auditAndFinish(
        HumanAccountCredentialAdministrationStatus::success,
        revoked.credential,
        revoked.actorId,
        revokedBrowserSessions,
        "allow",
        terminal
            ? "credential_already_terminal"
            : "credential_revoked",
        "success");
}
