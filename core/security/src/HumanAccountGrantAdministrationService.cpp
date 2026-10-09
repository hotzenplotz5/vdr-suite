#include "HumanAccountGrantAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "Database.h"
#include "HumanAccountAdministrationRepository.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <openssl/evp.h>
#include <sys/random.h>

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <ctime>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr std::size_t IdentifierBytes = 16;

bool systemEntropy(unsigned char* output, std::size_t size)
{
    if (output == nullptr || size == 0) return false;
    std::size_t offset = 0;
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

bool safeBackendScope(const std::string& value)
{
    if (value == "*") return true;
    if (value.empty() || value.size() > 128U) return false;

    return std::all_of(
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

const std::set<std::string>& supportedPermissionSet()
{
    static const std::set<std::string> permissions = {
        "role.admin",
        "role.read-only",
        "channels.view",
        "channels.move",
        "timers.view",
        "timers.create",
        "timers.modify",
        "timers.delete",
        "media.recording.play",
        "media.live.play",
        "remote.control",
        "osd.view",
        "osd.control",
        "recordings.view",
        "recordings.rename",
        "recordings.move",
        "recordings.delete",
        "recordings.cut",
        "recordings.marks.modify",
        "recordings.execute",
        "metadata.recording.assign",
        "searchtimers.create",
        "searchtimers.modify",
        "searchtimers.delete",
        "searchtimers.execute",
        "searchtimers.preview-cache.refresh",
        "broadcast.teletext.view",
        "broadcast.hbbtv.view",
        "broadcast.hbbtv.launch",
        "broadcast.hbbtv.input",
        "broadcast.session.manage_own",
        "broadcast.session.manage_all",
    };
    return permissions;
}

std::string hexEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    static constexpr char Hex[] =
        "0123456789abcdef";

    std::string result;
    result.reserve(size * 2U);
    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(
            Hex[(bytes[index] >> 4) & 0x0f]);
        result.push_back(
            Hex[bytes[index] & 0x0f]);
    }
    return result;
}

std::string grantSetRevision(
    const std::vector<PermissionGrant>& grants)
{
    std::string normalized = "grant-set/1\n";
    for (const PermissionGrant& grant : grants)
    {
        normalized += grant.permission;
        normalized += "\t";
        normalized += grant.backendId;
        normalized += "\n";
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLength = 0U;
    if (EVP_Digest(
            normalized.data(),
            normalized.size(),
            digest.data(),
            &digestLength,
            EVP_sha256(),
            nullptr) != 1 ||
        digestLength == 0U)
    {
        return {};
    }

    return "grant-set:" +
        hexEncode(digest.data(), digestLength);
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

bool grantTupleActive(
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
    DatabaseTransaction(
        Database& database,
        const char* beginStatement)
        : database_(database),
          active_(database_.execute(beginStatement))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const noexcept { return active_; }

    bool commit()
    {
        if (!active_ ||
            !database_.execute("COMMIT;"))
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

HumanAccountGrantAdministrationService::
HumanAccountGrantAdministrationService(
    Database& database,
    HumanAccountRepository& accountRepository,
    SecurityIdentityRepository& identityRepository,
    SecurityPermissionGrantRepository& grantRepository,
    HumanAccountAdministrationRepository& administrationRepository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      accountRepository_(accountRepository),
      identityRepository_(identityRepository),
      grantRepository_(grantRepository),
      administrationRepository_(administrationRepository),
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

std::string HumanAccountGrantAdministrationService::computeGrantSetRevision(
    const std::vector<PermissionGrant>& grants)
{
    return grantSetRevision(grants);
}

bool HumanAccountGrantAdministrationService::supportedGrant(
    const std::string& permission,
    const std::string& backendId)
{
    return safeBackendScope(backendId) &&
        supportedPermissionSet().count(permission) != 0U;
}

std::vector<std::string>
HumanAccountGrantAdministrationService::supportedGrantPermissions()
{
    const auto& permissions = supportedPermissionSet();
    return std::vector<std::string>(
        permissions.begin(),
        permissions.end());
}

std::vector<HumanAccountGrantOption>
HumanAccountGrantAdministrationService::supportedGrantPermissionOptions()
{
    std::vector<HumanAccountGrantOption> options;
    for (const std::string& permission : supportedPermissionSet())
    {
        HumanAccountGrantOption option;
        option.permission = permission;
        option.presentationKey = permission;
        option.category =
            permission.rfind("role.", 0U) == 0U
                ? "role"
                : "permission";
        options.push_back(std::move(option));
    }

    std::stable_sort(
        options.begin(),
        options.end(),
        [](const HumanAccountGrantOption& left,
           const HumanAccountGrantOption& right)
        {
            if (left.category != right.category)
                return left.category == "role";
            return left.permission < right.permission;
        });
    return options;
}

std::vector<std::string>
HumanAccountGrantAdministrationService::supportedGrantScopeKinds()
{
    return {"global", "backend"};
}

HumanAccountGrantAdministrationResult
HumanAccountGrantAdministrationService::readInActiveTransaction(
    const std::string& accountId) const
{
    HumanAccountGrantAdministrationResult result;

    const HumanAccountLookupResult account =
        accountRepository_.findByAccountId(accountId);
    if (account.status ==
        HumanAccountRepositoryStatus::notFound)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                accountNotFound;
        return result;
    }
    if (account.status !=
        HumanAccountRepositoryStatus::ok)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    const auto actor =
        identityRepository_.findActor(
            account.account.actorId);
    if (!actor.has_value() ||
        actor->type != ActorType::User ||
        !actor->active ||
        actor->revoked)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                accountActorInvalid;
        return result;
    }

    const SecurityPermissionGrantResolution resolution =
        grantRepository_.findActiveGrantsForActor(
            account.account.actorId);
    if (!resolution.available)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    result.grantSet.accountId =
        account.account.accountId;
    result.grantSet.actorId =
        account.account.actorId;

    for (const PermissionGrant& grant : resolution.grants)
    {
        if (supportedGrant(
                grant.permission,
                grant.backendId))
        {
            result.grantSet.grants.push_back(grant);
        }
    }

    std::sort(
        result.grantSet.grants.begin(),
        result.grantSet.grants.end(),
        [](const PermissionGrant& left,
           const PermissionGrant& right)
        {
            if (left.permission != right.permission)
            {
                return left.permission < right.permission;
            }
            return left.backendId < right.backendId;
        });

    result.grantSet.resourceRevision =
        grantSetRevision(result.grantSet.grants);
    if (result.grantSet.resourceRevision.empty())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    result.status =
        HumanAccountGrantAdministrationStatus::success;
    return result;
}

HumanAccountGrantAdministrationResult
HumanAccountGrantAdministrationService::read(
    const std::string& accountId) const
{
    HumanAccountGrantAdministrationResult result;
    if (!safeText(accountId, 128U))
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                accountNotFound;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_, "BEGIN;");
    if (!transaction.active())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    result = readInActiveTransaction(accountId);
    if (result.status ==
            HumanAccountGrantAdministrationStatus::success &&
        !transaction.commit())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
    }
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

    if (!safeText(context.actorId, 128U) ||
        !safeText(context.requestId, 128U) ||
        !safeText(context.correlationId, 128U, 0U) ||
        !safeText(accountId, 128U) ||
        expectedResourceRevision.empty() ||
        !supportedGrant(permission, backendId))
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                invalidGrant;
        return result;
    }

    const auto eventId =
        randomId(entropySource_, "ace_");
    const std::string occurredAt =
        formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(
        database_,
        "BEGIN IMMEDIATE;");
    if (!transaction.active())
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    const auto auditAndFinish =
        [&](HumanAccountGrantAdministrationStatus status,
            const HumanAccountGrantSet& grantSet,
            const std::string& decision,
            const std::string& reasonCode,
            const std::string& outcome)
        {
            HumanAccountGrantAdministrationResult finished;
            finished.status = status;
            finished.grantSet = grantSet;

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
            event.authenticationState =
                "authenticated";
            event.permission =
                "accounts.grants.modify";
            event.backendId = "*";
            event.operationId =
                "human-account-grant:" +
                accountId + ":" +
                permission + ":" +
                backendId;
            event.requestId = context.requestId;
            event.correlationId =
                context.correlationId;
            event.action =
                active
                    ? "human-account.grant.ensure"
                    : "human-account.grant.revoke";
            event.decision = decision;
            event.reasonCode = reasonCode;
            event.outcome = outcome;

            if (!accountabilityRepository_.append(event) ||
                !transaction.commit())
            {
                finished.status =
                    HumanAccountGrantAdministrationStatus::
                        storageError;
            }
            return finished;
        };

    result = readInActiveTransaction(accountId);
    if (result.status !=
        HumanAccountGrantAdministrationStatus::success)
    {
        return result;
    }

    const bool currentlyActive =
        grantTupleActive(
            result.grantSet.grants,
            permission,
            backendId);

    if (result.grantSet.resourceRevision !=
        expectedResourceRevision)
    {
        if (currentlyActive == active)
        {
            return auditAndFinish(
                HumanAccountGrantAdministrationStatus::
                    success,
                result.grantSet,
                "allow",
                active
                    ? "grant_already_active"
                    : "grant_already_absent",
                "success");
        }

        return auditAndFinish(
            HumanAccountGrantAdministrationStatus::
                revisionConflict,
            result.grantSet,
            "deny",
            "grant_set_revision_conflict",
            "failed");
    }

    if (currentlyActive == active)
    {
        return auditAndFinish(
            HumanAccountGrantAdministrationStatus::success,
            result.grantSet,
            "allow",
            active
                ? "grant_already_active"
                : "grant_already_absent",
            "success");
    }

    if (!active &&
        permission == "role.admin" &&
        backendId == "*")
    {
        const auto allAdmins =
            administrationRepository_.
                countUsableAdministratorsExcludingActor("");
        const auto otherAdmins =
            administrationRepository_.
                countUsableAdministratorsExcludingActor(
                    result.grantSet.actorId);
        if (!allAdmins.has_value() ||
            !otherAdmins.has_value())
        {
            result.status =
                HumanAccountGrantAdministrationStatus::
                    storageError;
            return result;
        }

        const bool targetIsUsableAdmin =
            *allAdmins > *otherAdmins;
        if (targetIsUsableAdmin &&
            *otherAdmins == 0U)
        {
            return auditAndFinish(
                HumanAccountGrantAdministrationStatus::
                    finalAdministrator,
                result.grantSet,
                "deny",
                "final_usable_administrator",
                "failed");
        }
    }

    const bool changed =
        active
            ? grantRepository_.ensureGrant(
                result.grantSet.actorId,
                permission,
                backendId)
            : grantRepository_.revokeGrant(
                result.grantSet.actorId,
                permission,
                backendId);
    if (!changed)
    {
        result.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return result;
    }

    HumanAccountGrantAdministrationResult updated =
        readInActiveTransaction(accountId);
    if (updated.status !=
        HumanAccountGrantAdministrationStatus::success)
    {
        return updated;
    }

    if (grantTupleActive(
            updated.grantSet.grants,
            permission,
            backendId) != active ||
        updated.grantSet.resourceRevision ==
            result.grantSet.resourceRevision)
    {
        updated.status =
            HumanAccountGrantAdministrationStatus::
                storageError;
        return updated;
    }

    return auditAndFinish(
        HumanAccountGrantAdministrationStatus::success,
        updated.grantSet,
        "allow",
        active
            ? "grant_ensured"
            : "grant_revoked",
        "success");
}
