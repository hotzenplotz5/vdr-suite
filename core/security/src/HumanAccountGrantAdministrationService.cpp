#include "HumanAccountGrantAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

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

bool concreteBackendId(const std::string& value)
{
    return !value.empty() &&
        value.size() <= 128U &&
        std::all_of(
            value.begin(),
            value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) ||
                    character == '.' ||
                    character == '_' ||
                    character == '-';
            });
}

bool supportedTarget(
    const std::string& permission,
    const std::string& backendId)
{
    return permission == "channels.view" &&
        concreteBackendId(backendId);
}

std::string hexEncode(
    const unsigned char* bytes,
    std::size_t size)
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
    const HumanAccountGrantAdministrationService::EntropySource&
        entropySource,
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
    if (gmtime_r(&timestamp, &utc) == nullptr) return {};

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

void appendRevisionPart(
    std::string& revision,
    const std::string& value)
{
    revision += std::to_string(value.size());
    revision += ":";
    revision += value;
    revision += "|";
}

std::string grantSetRevision(
    const std::string& accountId,
    const std::string& actorId,
    const std::vector<PermissionGrant>& grants)
{
    std::string revision = "account-grants-v1|";
    appendRevisionPart(revision, accountId);
    appendRevisionPart(revision, actorId);
    for (const PermissionGrant& grant : grants)
    {
        appendRevisionPart(revision, grant.permission);
        appendRevisionPart(revision, grant.backendId);
    }
    return revision;
}

bool grantActive(
    const std::vector<PermissionGrant>& grants,
    const std::string& permission,
    const std::string& backendId)
{
    return std::any_of(
        grants.begin(),
        grants.end(),
        [&](const PermissionGrant& grant)
        {
            return grant.permission == permission &&
                grant.backendId == backendId;
        });
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
    const HumanAccountGrantAdministrationContext& context)
{
    return safeText(context.actorId, 128U) &&
        safeText(context.requestId, 128U) &&
        safeText(context.correlationId, 128U, 0U) &&
        context.actorType != ActorType::Anonymous;
}

AccountabilityEvent eventFor(
    const HumanAccountGrantAdministrationContext& context,
    const std::string& eventId,
    const std::string& occurredAt,
    const std::string& accountId,
    const std::string& permission,
    const std::string& backendId,
    bool active,
    const std::string& decision,
    const std::string& reasonCode,
    const std::string& outcome)
{
    AccountabilityEvent event;
    event.eventId = eventId;
    event.classes = "audit,security,identity";
    event.eventType =
        "security.human-account.grant-administration";
    event.severity = decision == "allow" ? "info" : "warning";
    event.occurredAt = occurredAt;
    event.actorId = context.actorId;
    event.actorType = actorTypeName(context.actorType);
    event.authenticationState = "authenticated";
    event.permission = "accounts.grants.modify";
    event.backendId = backendId;
    event.operationId =
        "human-account-grant:" + accountId + ":" +
        permission + ":" + backendId;
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = active
        ? "human-account.grant.ensure"
        : "human-account.grant.revoke";
    event.decision = decision;
    event.reasonCode = reasonCode;
    event.outcome = outcome;
    return event;
}
}

HumanAccountGrantAdministrationService::
HumanAccountGrantAdministrationService(
    Database& database,
    HumanAccountRepository& accountRepository,
    SecurityIdentityRepository& identityRepository,
    SecurityPermissionGrantRepository& grantRepository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      accountRepository_(accountRepository),
      identityRepository_(identityRepository),
      grantRepository_(grantRepository),
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

HumanAccountGrantAdministrationResult
HumanAccountGrantAdministrationService::read(
    const std::string& accountId) const
{
    HumanAccountGrantAdministrationResult result;
    if (!safeText(accountId, 128U))
    {
        result.status =
            HumanAccountGrantAdministrationStatus::invalidRequest;
        return result;
    }

    const HumanAccountLookupResult account =
        accountRepository_.findByAccountId(accountId);
    if (account.status == HumanAccountRepositoryStatus::notFound)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::accountNotFound;
        return result;
    }
    if (account.status == HumanAccountRepositoryStatus::invalid)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::invalidRequest;
        return result;
    }
    if (account.status != HumanAccountRepositoryStatus::ok)
    {
        return result;
    }

    const auto actor =
        identityRepository_.findActor(account.account.actorId);
    if (!actor.has_value() ||
        actor->type != ActorType::User ||
        !actor->active ||
        actor->revoked)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::accountActorInvalid;
        return result;
    }

    const SecurityPermissionGrantResolution stored =
        grantRepository_.findActiveGrantsForActor(
            account.account.actorId);
    if (!stored.available)
    {
        return result;
    }

    result.resource.accountId = account.account.accountId;
    result.resource.actorId = account.account.actorId;
    for (const PermissionGrant& grant : stored.grants)
    {
        if (supportedTarget(
                grant.permission,
                grant.backendId))
        {
            result.resource.grants.push_back(grant);
        }
    }
    std::sort(
        result.resource.grants.begin(),
        result.resource.grants.end(),
        [](const PermissionGrant& left,
           const PermissionGrant& right)
        {
            if (left.permission != right.permission)
                return left.permission < right.permission;
            return left.backendId < right.backendId;
        });
    result.resource.resourceRevision =
        grantSetRevision(
            result.resource.accountId,
            result.resource.actorId,
            result.resource.grants);
    result.status =
        HumanAccountGrantAdministrationStatus::success;
    return result;
}

HumanAccountGrantAdministrationResult
HumanAccountGrantAdministrationService::setGrant(
    const HumanAccountGrantAdministrationContext& context,
    const std::string& accountId,
    const std::string& expectedResourceRevision,
    const std::string& permission,
    const std::string& backendId,
    bool active)
{
    HumanAccountGrantAdministrationResult result;
    if (!validContext(context) ||
        !safeText(accountId, 128U) ||
        !safeText(expectedResourceRevision, 4096U) ||
        !supportedTarget(permission, backendId))
    {
        result.status =
            HumanAccountGrantAdministrationStatus::invalidRequest;
        return result;
    }

    const auto eventId =
        randomId(entropySource_, "ace_");
    const std::string occurredAt =
        formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty()) return result;

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) return result;

    const auto audited = [&](
        HumanAccountGrantAdministrationResult value,
        const std::string& decision,
        const std::string& reasonCode,
        const std::string& outcome)
    {
        const AccountabilityEvent event = eventFor(
            context,
            *eventId,
            occurredAt,
            accountId,
            permission,
            backendId,
            active,
            decision,
            reasonCode,
            outcome);
        if (!accountabilityRepository_.append(event) ||
            !transaction.commit())
        {
            value.status =
                HumanAccountGrantAdministrationStatus::storageError;
        }
        return value;
    };

    HumanAccountGrantAdministrationResult current =
        read(accountId);
    if (current.status ==
        HumanAccountGrantAdministrationStatus::accountNotFound)
    {
        return audited(
            current,
            "deny",
            "account_not_found",
            "failed");
    }
    if (current.status ==
        HumanAccountGrantAdministrationStatus::accountActorInvalid)
    {
        return audited(
            current,
            "deny",
            "account_actor_invalid",
            "failed");
    }
    if (current.status !=
        HumanAccountGrantAdministrationStatus::success)
    {
        return result;
    }

    const bool currentlyActive =
        grantActive(
            current.resource.grants,
            permission,
            backendId);

    if (current.resource.resourceRevision !=
            expectedResourceRevision &&
        currentlyActive != active)
    {
        current.status =
            HumanAccountGrantAdministrationStatus::revisionConflict;
        return audited(
            current,
            "deny",
            "grant_set_revision_conflict",
            "failed");
    }

    if (currentlyActive == active)
    {
        current.status =
            HumanAccountGrantAdministrationStatus::success;
        return audited(
            current,
            "allow",
            active
                ? "grant_already_active"
                : "grant_already_revoked",
            "success");
    }

    const bool persisted = active
        ? grantRepository_.ensureGrant(
            current.resource.actorId,
            permission,
            backendId)
        : grantRepository_.revokeGrant(
            current.resource.actorId,
            permission,
            backendId);
    if (!persisted)
    {
        return result;
    }

    HumanAccountGrantAdministrationResult updated =
        read(accountId);
    if (updated.status !=
            HumanAccountGrantAdministrationStatus::success ||
        grantActive(
            updated.resource.grants,
            permission,
            backendId) != active ||
        updated.resource.resourceRevision ==
            current.resource.resourceRevision)
    {
        return result;
    }

    return audited(
        updated,
        "allow",
        active ? "grant_ensured" : "grant_revoked",
        "success");
}
