#include "HumanAccountRecoveryService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionCredentialRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountRepository.h"
#include "SecurityIdentityRepository.h"

#include <crypt.h>
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
constexpr std::size_t PasswordSaltBytes = 32;

void secureWipe(std::string& value) noexcept
{
    volatile char* bytes = value.empty()
        ? nullptr
        : const_cast<volatile char*>(value.data());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        bytes[index] = 0;
    }
    value.clear();
}

template <typename Value>
void secureWipeObject(Value& value) noexcept
{
    volatile unsigned char* bytes =
        reinterpret_cast<volatile unsigned char*>(&value);
    for (std::size_t index = 0; index < sizeof(Value); ++index)
    {
        bytes[index] = 0;
    }
}

class RequestSecretGuard
{
public:
    explicit RequestSecretGuard(HumanAccountRecoveryRequest& request)
        : request_(request)
    {
    }

    ~RequestSecretGuard()
    {
        request_.clearSecrets();
    }

private:
    HumanAccountRecoveryRequest& request_;
};

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
    const HumanAccountRecoveryService::EntropySource& entropySource,
    const std::string& prefix)
{
    std::array<unsigned char, IdentifierBytes> bytes{};
    if (!entropySource ||
        !entropySource(bytes.data(), bytes.size()))
    {
        secureWipeObject(bytes);
        return std::nullopt;
    }

    std::string result =
        prefix + hexEncode(bytes.data(), bytes.size());
    secureWipeObject(bytes);
    return result;
}

std::string hashHumanPassword(
    const HumanAccountRecoveryService::EntropySource& entropySource,
    const std::string& password)
{
    std::array<unsigned char, PasswordSaltBytes> saltBytes{};
    if (!entropySource ||
        !entropySource(saltBytes.data(), saltBytes.size()))
    {
        secureWipeObject(saltBytes);
        return {};
    }

    std::array<char, 256> setting{};
    char* generated = crypt_gensalt_rn(
        "$y$",
        0,
        reinterpret_cast<const char*>(saltBytes.data()),
        static_cast<int>(saltBytes.size()),
        setting.data(),
        static_cast<int>(setting.size()));
    secureWipeObject(saltBytes);

    if (generated == nullptr)
    {
        secureWipeObject(setting);
        return {};
    }

    crypt_data data{};
    char* encoded = crypt_r(
        password.c_str(),
        generated,
        &data);

    std::string result;
    if (encoded != nullptr &&
        encoded[0] != '*' &&
        std::string(encoded).rfind("$y$", 0) == 0)
    {
        result = encoded;
    }

    secureWipeObject(data);
    secureWipeObject(setting);
    return result;
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
}

void HumanAccountRecoveryRequest::clearSecrets() noexcept
{
    secureWipe(newPassword);
}

HumanAccountRecoveryService::HumanAccountRecoveryService(
    Database& database,
    HumanAccountRepository& humanAccountRepository,
    CredentialVerifierRepository& credentialVerifierRepository,
    SecurityIdentityRepository& identityRepository,
    BrowserSessionCredentialRepository& browserSessionCredentialRepository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      humanAccountRepository_(humanAccountRepository),
      credentialVerifierRepository_(credentialVerifierRepository),
      identityRepository_(identityRepository),
      browserSessionCredentialRepository_(browserSessionCredentialRepository),
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

HumanAccountRecoveryResult HumanAccountRecoveryService::recover(
    HumanAccountRecoveryRequest request)
{
    HumanAccountRecoveryResult result;
    RequestSecretGuard secretGuard(request);

    if (!safeText(request.accountId, 128) ||
        !safeText(request.loginName, 128) ||
        !safeText(request.newPassword, 1024) ||
        !safeText(request.requestId, 128) ||
        !safeText(request.correlationId, 128, 0))
    {
        result.status = HumanAccountRecoveryStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_, "ace_");
    const auto replacementCredentialId =
        randomId(entropySource_, "credential_");
    if (!eventId.has_value() ||
        !replacementCredentialId.has_value())
    {
        result.status = HumanAccountRecoveryStatus::entropyUnavailable;
        return result;
    }

    const std::string occurredAt =
        formatTimestamp(clock_());
    if (occurredAt.empty())
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    std::string targetActorId;

    const auto appendOutcome = [&](
        const std::string& decision,
        const std::string& reasonCode,
        const std::string& outcome) -> bool
    {
        AccountabilityEvent event;
        event.eventId = *eventId;
        event.classes = "security,identity,recovery";
        event.eventType = "security.human-account.recovery";
        event.severity = decision == "allow"
            ? "info"
            : "warning";
        event.occurredAt = occurredAt;
        event.actorId = "system:human-account-recovery";
        event.actorType = "system";
        event.authenticationState = "local-root";
        event.permission = "";
        event.backendId = "*";
        event.operationId =
            "human-account-recovery:" + request.accountId;
        event.requestId = request.requestId;
        event.correlationId = request.correlationId;
        event.action = "human-account.credential.rotate";
        event.decision = decision;
        event.reasonCode = reasonCode;
        event.outcome = outcome;
        return accountabilityRepository_.append(event);
    };

    const auto failAudited = [&](
        HumanAccountRecoveryStatus status,
        const std::string& reasonCode)
        -> HumanAccountRecoveryResult
    {
        HumanAccountRecoveryResult failure;
        failure.status = status;
        failure.accountId = request.accountId;
        failure.actorId = targetActorId;
        if (!appendOutcome("deny", reasonCode, "failed") ||
            !transaction.commit())
        {
            failure.status =
                HumanAccountRecoveryStatus::storageError;
        }
        return failure;
    };

    const HumanAccountLookupResult account =
        humanAccountRepository_.findByAccountId(
            request.accountId);
    if (account.status == HumanAccountRepositoryStatus::notFound)
    {
        return failAudited(
            HumanAccountRecoveryStatus::accountNotFound,
            "account_not_found");
    }
    if (account.status != HumanAccountRepositoryStatus::ok)
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    targetActorId = account.account.actorId;
    const auto actor =
        identityRepository_.findActor(targetActorId);
    if (!actor.has_value() ||
        actor->actorId != targetActorId ||
        actor->type != ActorType::User)
    {
        return failAudited(
            HumanAccountRecoveryStatus::actorInvalid,
            "account_actor_invalid");
    }
    if (!account.account.active ||
        !actor->active ||
        actor->revoked)
    {
        return failAudited(
            HumanAccountRecoveryStatus::accountInactive,
            "account_inactive");
    }

    const auto verifier =
        credentialVerifierRepository_.findByLogin(
            request.loginName);
    if (!verifier.has_value())
    {
        return failAudited(
            HumanAccountRecoveryStatus::credentialNotFound,
            "credential_not_found");
    }

    const auto credential =
        identityRepository_.findCredential(
            verifier->credentialId);
    if (!credential.has_value() ||
        credential->actorId != targetActorId ||
        credential->credentialType != "human-password" ||
        !credential->active ||
        credential->expired ||
        credential->revoked)
    {
        return failAudited(
            HumanAccountRecoveryStatus::credentialInvalid,
            "credential_invalid");
    }

    const auto browserSessions =
        browserSessionCredentialRepository_.
            findByIssuedFromCredentialId(
                credential->credentialId);
    if (!browserSessions.has_value())
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    std::string passwordHash =
        hashHumanPassword(
            entropySource_,
            request.newPassword);
    secureWipe(request.newPassword);
    if (passwordHash.empty())
    {
        return failAudited(
            HumanAccountRecoveryStatus::hashingUnavailable,
            "password_hashing_unavailable");
    }

    if (!identityRepository_.rotateCredentialInActiveTransaction(
            credential->credentialId,
            *replacementCredentialId,
            targetActorId,
            "human-password"))
    {
        secureWipe(passwordHash);
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    const bool verifierRotated =
        credentialVerifierRepository_.rotateVerifierInActiveTransaction(
            credential->credentialId,
            *replacementCredentialId,
            request.loginName,
            passwordHash);
    secureWipe(passwordHash);
    if (!verifierRotated)
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    std::size_t revokedSessions = 0;
    for (const StoredBrowserSessionCredential& browserSession :
         *browserSessions)
    {
        if (!browserSessionCredentialRepository_.revokeBySessionId(
                browserSession.sessionId) ||
            !identityRepository_.revokeSession(
                browserSession.sessionId) ||
            !identityRepository_.revokeCredential(
                browserSession.credentialId))
        {
            result.status = HumanAccountRecoveryStatus::storageError;
            return result;
        }
        ++revokedSessions;
    }

    if (!appendOutcome(
            "allow",
            "local_root_password_rotation",
            "success"))
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    if (!transaction.commit())
    {
        result.status = HumanAccountRecoveryStatus::storageError;
        return result;
    }

    result.status = HumanAccountRecoveryStatus::success;
    result.accountId = account.account.accountId;
    result.actorId = targetActorId;
    result.replacedCredentialId = credential->credentialId;
    result.credentialId = *replacementCredentialId;
    result.revokedBrowserSessions = revokedSessions;
    return result;
}
