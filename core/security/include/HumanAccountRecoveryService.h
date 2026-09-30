#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionCredentialRepository;
class CredentialVerifierRepository;
class Database;
class HumanAccountRepository;
class SecurityIdentityRepository;

enum class HumanAccountRecoveryStatus
{
    success,
    invalidRequest,
    accountNotFound,
    accountInactive,
    actorInvalid,
    credentialNotFound,
    credentialInvalid,
    entropyUnavailable,
    hashingUnavailable,
    storageError,
};

struct HumanAccountRecoveryRequest
{
    std::string accountId;
    std::string loginName;
    std::string newPassword;
    std::string requestId;
    std::string correlationId;

    void clearSecrets() noexcept;
};

struct HumanAccountRecoveryResult
{
    HumanAccountRecoveryStatus status =
        HumanAccountRecoveryStatus::storageError;
    std::string accountId;
    std::string actorId;
    std::string credentialId;
    std::size_t revokedBrowserSessions = 0;
};

class HumanAccountRecoveryService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountRecoveryService(
        Database& database,
        HumanAccountRepository& humanAccountRepository,
        CredentialVerifierRepository& credentialVerifierRepository,
        SecurityIdentityRepository& identityRepository,
        BrowserSessionCredentialRepository& browserSessionCredentialRepository,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountRecoveryResult recover(
        HumanAccountRecoveryRequest request);

private:
    Database& database_;
    HumanAccountRepository& humanAccountRepository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionCredentialRepository& browserSessionCredentialRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
