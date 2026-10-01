#include "HumanAccountAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionLifecycleService.h"
#include "Database.h"
#include "SecurityIdentityRepository.h"

#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstddef>
#include <ctime>
#include <optional>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16;

bool systemEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0)
    {
        return false;
    }

    std::size_t offset = 0;
    while (offset < size)
    {
        const ssize_t received =
            getrandom(output + offset, size - offset, 0);
        if (received < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            return false;
        }
        if (received == 0)
        {
            return false;
        }
        offset += static_cast<std::size_t>(received);
    }
    return true;
}

bool safeText(
    const std::string& value,
    std::size_t maximumLength,
    std::size_t minimumLength = 1)
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

std::string hexEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2);

    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(Hex[(bytes[index] >> 4) & 0x0f]);
        result.push_back(Hex[bytes[index] & 0x0f]);
    }
    return result;
}

std::optional<std::string> randomId(
    const HumanAccountAdministrationService::EntropySource& entropySource,
    const std::string& prefix)
{
    std::array<unsigned char, IdentifierBytes> bytes{};
    if (!entropySource ||
        !entropySource(bytes.data(), bytes.size()))
    {
        return std::nullopt;
    }

    return prefix + hexEncode(bytes.data(), bytes.size());
}

std::string formatTimestamp(
    std::chrono::system_clock::time_point value)
{
    const std::time_t timestamp =
        std::chrono::system_clock::to_time_t(value);
    std::tm utc{};
    if (gmtime_r(&timestamp, &utc) == nullptr)
    {
        return {};
    }

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

bool validContext(
    const HumanAccountAdministrationContext& context)
{
    return safeText(context.actorId, 128) &&
        safeText(context.requestId, 128) &&
        safeText(context.correlationId, 128, 0) &&
        context.actorType != ActorType::Anonymous;
}

AccountabilityEvent eventFor(
    const HumanAccountAdministrationContext& context,
    const std::string& eventId,
    const std::string& occurredAt,
    const std::string& accountId,
    std::uint64_t expectedRevision,
    const std::string& permission,
    const std::string& action,
    const std::string& decision,
    const std::string& reasonCode,
    const std::string& outcome)
{
    AccountabilityEvent event;
    event.eventId = eventId;
    event.classes = "audit,security,identity";
    event.eventType = "security.human-account.administration";
    event.severity = decision == "allow"
        ? "info"
        : "warning";
    event.occurredAt = occurredAt;
    event.actorId = context.actorId;
    event.actorType = actorTypeName(context.actorType);
    event.authenticationState = "authenticated";
    event.permission = permission;
    event.backendId = "*";
    event.operationId =
        "human-account:" + accountId +
        ":revision:" + std::to_string(expectedRevision);
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = action;
    event.decision = decision;
    event.reasonCode = reasonCode;
    event.outcome = outcome;
    return event;
}
}

HumanAccountAdministrationService::HumanAccountAdministrationService(
    Database& database,
    HumanAccountRepository& accountRepository,
    SecurityIdentityRepository& identityRepository,
    BrowserSessionLifecycleService& browserSessionLifecycleService,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      accountRepository_(accountRepository),
      identityRepository_(identityRepository),
      browserSessionLifecycleService_(browserSessionLifecycleService),
      accountabilityRepository_(accountabilityRepository),
      entropySource_(entropySource
          ? std::move(entropySource)
          : EntropySource(systemEntropy)),
      clock_(clock
          ? std::move(clock)
          : Clock([]
            {
                return std::chrono::system_clock::now();
            }))
{
}

HumanAccountAdministrationResult
HumanAccountAdministrationService::modifyDisplayName(
    const HumanAccountAdministrationContext& context,
    const std::string& accountId,
    std::uint64_t expectedRevision,
    const std::string& displayName)
{
    HumanAccountAdministrationResult result;
    if (!validContext(context) ||
        !safeText(accountId, 128) ||
        !safeText(displayName, 256) ||
        expectedRevision == 0)
    {
        result.status =
            HumanAccountAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_, "ace_");
    const std::string occurredAt = formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    const auto auditedFailure = [&](
        HumanAccountAdministrationStatus status,
        const std::string& reasonCode)
    {
        HumanAccountAdministrationResult failure;
        failure.status = status;
        const HumanAccountLookupResult current =
            accountRepository_.findByAccountId(accountId);
        if (current.status == HumanAccountRepositoryStatus::ok)
        {
            failure.account = current.account;
        }

        const AccountabilityEvent event = eventFor(
            context,
            *eventId,
            occurredAt,
            accountId,
            expectedRevision,
            "accounts.modify",
            "human-account.modify",
            "deny",
            reasonCode,
            "failed");
        if (!accountabilityRepository_.append(event) ||
            !transaction.commit())
        {
            failure.status =
                HumanAccountAdministrationStatus::storageError;
        }
        return failure;
    };

    const HumanAccountLookupResult account =
        accountRepository_.findByAccountId(accountId);
    if (account.status == HumanAccountRepositoryStatus::notFound)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::accountNotFound,
            "account_not_found");
    }
    if (account.status != HumanAccountRepositoryStatus::ok)
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }
    if (account.account.revision != expectedRevision)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::revisionConflict,
            "account_revision_conflict");
    }

    const auto actor =
        identityRepository_.findActor(account.account.actorId);
    if (!actor.has_value() ||
        actor->type != ActorType::User ||
        !actor->active ||
        actor->revoked)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::accountActorInvalid,
            "account_actor_invalid");
    }

    bool changed = false;
    if (account.account.displayName != displayName ||
        actor->displayName != displayName)
    {
        if (accountRepository_.updateDisplayNameInActiveTransaction(
                accountId,
                expectedRevision,
                displayName) != HumanAccountRepositoryStatus::ok ||
            !identityRepository_.updateActorDisplayNameInActiveTransaction(
                account.account.actorId,
                displayName))
        {
            result.status =
                HumanAccountAdministrationStatus::storageError;
            return result;
        }
        changed = true;
    }

    const HumanAccountLookupResult updated =
        accountRepository_.findByAccountId(accountId);
    if (updated.status != HumanAccountRepositoryStatus::ok ||
        updated.account.displayName != displayName ||
        updated.account.revision !=
            expectedRevision + (changed ? 1U : 0U))
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    const AccountabilityEvent event = eventFor(
        context,
        *eventId,
        occurredAt,
        accountId,
        expectedRevision,
        "accounts.modify",
        "human-account.modify",
        "allow",
        changed
            ? "account_display_name_updated"
            : "account_display_name_already_current",
        "success");
    if (!accountabilityRepository_.append(event) ||
        !transaction.commit())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    result.status = HumanAccountAdministrationStatus::success;
    result.account = updated.account;
    return result;
}

HumanAccountAdministrationResult
HumanAccountAdministrationService::setActive(
    const HumanAccountAdministrationContext& context,
    const std::string& accountId,
    std::uint64_t expectedRevision,
    bool active)
{
    HumanAccountAdministrationResult result;
    if (!validContext(context) ||
        !safeText(accountId, 128) ||
        expectedRevision == 0)
    {
        result.status =
            HumanAccountAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_, "ace_");
    const std::string occurredAt = formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    const std::string permission =
        active ? "accounts.activate" : "accounts.deactivate";
    const std::string action =
        active ? "human-account.activate" : "human-account.deactivate";

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    const auto auditedFailure = [&](
        HumanAccountAdministrationStatus status,
        const std::string& reasonCode)
    {
        HumanAccountAdministrationResult failure;
        failure.status = status;
        const HumanAccountLookupResult current =
            accountRepository_.findByAccountId(accountId);
        if (current.status == HumanAccountRepositoryStatus::ok)
        {
            failure.account = current.account;
        }

        const AccountabilityEvent event = eventFor(
            context,
            *eventId,
            occurredAt,
            accountId,
            expectedRevision,
            permission,
            action,
            "deny",
            reasonCode,
            "failed");
        if (!accountabilityRepository_.append(event) ||
            !transaction.commit())
        {
            failure.status =
                HumanAccountAdministrationStatus::storageError;
        }
        return failure;
    };

    const HumanAccountLookupResult account =
        accountRepository_.findByAccountId(accountId);
    if (account.status == HumanAccountRepositoryStatus::notFound)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::accountNotFound,
            "account_not_found");
    }
    if (account.status != HumanAccountRepositoryStatus::ok)
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }
    if (account.account.revision != expectedRevision)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::revisionConflict,
            "account_revision_conflict");
    }

    const auto actor =
        identityRepository_.findActor(account.account.actorId);
    if (!actor.has_value() ||
        actor->type != ActorType::User ||
        !actor->active ||
        actor->revoked)
    {
        return auditedFailure(
            HumanAccountAdministrationStatus::accountActorInvalid,
            "account_actor_invalid");
    }

    std::size_t revokedSessions = 0;
    bool changed = false;

    if (account.account.active != active)
    {
        if (!active)
        {
            const auto allAdmins =
                accountRepository_.
                    countUsableAdministratorsExcludingActor("");
            const auto otherAdmins =
                accountRepository_.
                    countUsableAdministratorsExcludingActor(
                        account.account.actorId);
            if (!allAdmins.has_value() ||
                !otherAdmins.has_value())
            {
                result.status =
                    HumanAccountAdministrationStatus::storageError;
                return result;
            }

            const bool targetIsUsableAdmin =
                *allAdmins > *otherAdmins;
            if (targetIsUsableAdmin && *otherAdmins == 0)
            {
                return auditedFailure(
                    HumanAccountAdministrationStatus::finalAdministrator,
                    "final_usable_administrator");
            }
        }

        if (accountRepository_.setActiveInActiveTransaction(
                accountId,
                expectedRevision,
                active) != HumanAccountRepositoryStatus::ok)
        {
            result.status =
                HumanAccountAdministrationStatus::storageError;
            return result;
        }
        changed = true;
    }

    if (!active)
    {
        const auto revoked =
            browserSessionLifecycleService_.
                revokeAllForActorInActiveTransaction(
                    account.account.actorId);
        if (!revoked.has_value())
        {
            result.status =
                HumanAccountAdministrationStatus::storageError;
            return result;
        }
        revokedSessions = *revoked;
    }

    const HumanAccountLookupResult updated =
        accountRepository_.findByAccountId(accountId);
    if (updated.status != HumanAccountRepositoryStatus::ok ||
        updated.account.active != active ||
        updated.account.revision !=
            expectedRevision + (changed ? 1U : 0U))
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    const AccountabilityEvent event = eventFor(
        context,
        *eventId,
        occurredAt,
        accountId,
        expectedRevision,
        permission,
        action,
        "allow",
        changed
            ? (active
                ? "account_activated"
                : "account_deactivated")
            : (active
                ? "account_already_active"
                : "account_already_inactive_sessions_fenced"),
        "success");
    if (!accountabilityRepository_.append(event) ||
        !transaction.commit())
    {
        result.status =
            HumanAccountAdministrationStatus::storageError;
        return result;
    }

    result.status = HumanAccountAdministrationStatus::success;
    result.account = updated.account;
    result.revokedBrowserSessions = revokedSessions;
    return result;
}
