#include "HumanAccountCreationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "HumanAccountCreationRepository.h"
#include "SecurityIdentityProvisioningRepository.h"

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

bool validIdempotencyKey(const std::string& value)
{
    if (value.empty() || value.size() > 160U)
    {
        return false;
    }
    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return character >= 0x21U && character <= 0x7eU;
        });
}

std::string hexEncode(
    const unsigned char* bytes,
    std::size_t size)
{
    static constexpr char Hex[] = "0123456789abcdef";
    std::string result;
    result.reserve(size * 2U);

    for (std::size_t index = 0; index < size; ++index)
    {
        result.push_back(Hex[(bytes[index] >> 4U) & 0x0fU]);
        result.push_back(Hex[bytes[index] & 0x0fU]);
    }
    return result;
}

std::optional<std::string> randomId(
    const HumanAccountCreationService::EntropySource& entropySource,
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
    const HumanAccountCreationService::EntropySource& entropySource,
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

AccountabilityEvent createEvent(
    const HumanAccountCreationRequest& request,
    const std::string& eventId,
    const std::string& occurredAt,
    const std::string& accountId,
    const std::string& decision,
    const std::string& reasonCode,
    const std::string& outcome)
{
    AccountabilityEvent event;
    event.eventId = eventId;
    event.classes = "audit,security,identity";
    event.eventType = "security.human-account.create";
    event.severity = decision == "allow" ? "info" : "warning";
    event.occurredAt = occurredAt;
    event.actorId = request.actorId;
    event.actorType = actorTypeName(request.actorType);
    event.authenticationState = "authenticated";
    event.permission = "accounts.create";
    event.backendId = "*";
    event.operationId = accountId.empty()
        ? "human-account:create"
        : "human-account:" + accountId + ":create";
    event.requestId = request.requestId;
    event.correlationId = request.correlationId;
    event.action = "human-account.create";
    event.decision = decision;
    event.reasonCode = reasonCode;
    event.outcome = outcome;
    return event;
}
}

void HumanAccountCreationRequest::clearSecrets() noexcept
{
    secureWipe(password);
}

HumanAccountCreationService::HumanAccountCreationService(
    Database& database,
    SecurityIdentityProvisioningRepository& identityProvisioningRepository,
    HumanAccountRepository& humanAccountRepository,
    HumanAccountCreationRepository& creationRepository,
    CredentialVerifierRepository& credentialVerifierRepository,
    AccountabilityEventRepository& accountabilityRepository,
    EntropySource entropySource,
    Clock clock)
    : database_(database),
      identityProvisioningRepository_(identityProvisioningRepository),
      humanAccountRepository_(humanAccountRepository),
      creationRepository_(creationRepository),
      credentialVerifierRepository_(credentialVerifierRepository),
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

HumanAccountCreationResult HumanAccountCreationService::create(
    HumanAccountCreationRequest request)
{
    HumanAccountCreationResult result;

    if (!safeText(request.actorId, 128) ||
        request.actorType == ActorType::Anonymous ||
        !validIdempotencyKey(request.idempotencyKey) ||
        !safeText(request.loginName, 128) ||
        !safeText(request.password, 1024) ||
        !safeText(request.displayName, 256) ||
        !safeText(request.requestId, 128) ||
        !safeText(request.correlationId, 128, 0))
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::invalidRequest;
        return result;
    }

    const auto eventId = randomId(entropySource_, "ace_");
    const std::string occurredAt = formatTimestamp(clock_());
    if (!eventId.has_value())
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::entropyUnavailable;
        return result;
    }
    if (occurredAt.empty())
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active())
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }

    const HumanAccountCreateIdempotencyLookupResult existing =
        creationRepository_.find(
            request.actorId,
            request.idempotencyKey);

    if (existing.status == HumanAccountCreateIdempotencyStatus::ok)
    {
        request.clearSecrets();

        if (existing.record.loginName != request.loginName ||
            existing.record.displayName != request.displayName)
        {
            if (!accountabilityRepository_.append(
                    createEvent(
                        request,
                        *eventId,
                        occurredAt,
                        existing.record.accountId,
                        "deny",
                        "idempotency_conflict",
                        "failed")) ||
                !transaction.commit())
            {
                result.status = HumanAccountCreationStatus::storageError;
                return result;
            }

            result.status =
                HumanAccountCreationStatus::idempotencyConflict;
            return result;
        }

        const HumanAccountLookupResult stored =
            humanAccountRepository_.findByAccountId(
                existing.record.accountId);
        if (stored.status != HumanAccountRepositoryStatus::ok)
        {
            result.status = HumanAccountCreationStatus::storageError;
            return result;
        }

        if (!accountabilityRepository_.append(
                createEvent(
                    request,
                    *eventId,
                    occurredAt,
                    stored.account.accountId,
                    "allow",
                    "account_create_replayed",
                    "success")) ||
            !transaction.commit())
        {
            result.status = HumanAccountCreationStatus::storageError;
            return result;
        }

        result.status = HumanAccountCreationStatus::replayed;
        result.account = stored.account;
        return result;
    }

    if (existing.status != HumanAccountCreateIdempotencyStatus::notFound)
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }

    if (credentialVerifierRepository_.findByLogin(
            request.loginName).has_value())
    {
        request.clearSecrets();
        if (!accountabilityRepository_.append(
                createEvent(
                    request,
                    *eventId,
                    occurredAt,
                    "",
                    "deny",
                    "login_name_conflict",
                    "failed")) ||
            !transaction.commit())
        {
            result.status = HumanAccountCreationStatus::storageError;
            return result;
        }

        result.status = HumanAccountCreationStatus::loginConflict;
        return result;
    }

    const auto accountId = randomId(entropySource_, "account_");
    const auto actorId = randomId(entropySource_, "actor_");
    const auto credentialId = randomId(entropySource_, "credential_");
    if (!accountId.has_value() ||
        !actorId.has_value() ||
        !credentialId.has_value())
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::entropyUnavailable;
        return result;
    }

    std::string passwordHash =
        hashHumanPassword(entropySource_, request.password);
    secureWipe(request.password);
    if (passwordHash.empty())
    {
        request.clearSecrets();
        result.status = HumanAccountCreationStatus::hashingUnavailable;
        return result;
    }

    if (!identityProvisioningRepository_.
            ensureHumanCredentialInActiveTransaction(
                *actorId,
                request.displayName,
                *credentialId,
                "human-password") ||
        !humanAccountRepository_.ensureAccountInActiveTransaction(
            *accountId,
            *actorId,
            request.displayName) ||
        !credentialVerifierRepository_.ensureVerifier(
            *credentialId,
            request.loginName,
            passwordHash))
    {
        secureWipe(passwordHash);
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }
    secureWipe(passwordHash);

    HumanAccountCreateIdempotencyRecord binding;
    binding.requestActorId = request.actorId;
    binding.idempotencyKey = request.idempotencyKey;
    binding.loginName = request.loginName;
    binding.displayName = request.displayName;
    binding.accountId = *accountId;

    const HumanAccountCreateIdempotencyStatus recorded =
        creationRepository_.recordInActiveTransaction(binding);
    if (recorded != HumanAccountCreateIdempotencyStatus::ok)
    {
        result.status =
            recorded == HumanAccountCreateIdempotencyStatus::conflict
                ? HumanAccountCreationStatus::idempotencyConflict
                : HumanAccountCreationStatus::storageError;
        return result;
    }

    const HumanAccountLookupResult created =
        humanAccountRepository_.findByAccountId(*accountId);
    if (created.status != HumanAccountRepositoryStatus::ok ||
        created.account.actorId != *actorId ||
        created.account.displayName != request.displayName ||
        !created.account.active ||
        created.account.revision != 1U)
    {
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }

    if (!accountabilityRepository_.append(
            createEvent(
                request,
                *eventId,
                occurredAt,
                created.account.accountId,
                "allow",
                "account_created",
                "success")) ||
        !transaction.commit())
    {
        result.status = HumanAccountCreationStatus::storageError;
        return result;
    }

    result.status = HumanAccountCreationStatus::success;
    result.account = created.account;
    return result;
}
