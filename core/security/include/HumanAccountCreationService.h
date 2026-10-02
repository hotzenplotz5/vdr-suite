#pragma once

#include "HumanAccountRepository.h"

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class CredentialVerifierRepository;
class Database;
class HumanAccountCreationRepository;
class SecurityIdentityProvisioningRepository;

enum class HumanAccountCreationStatus
{
    success,
    replayed,
    invalidRequest,
    loginConflict,
    idempotencyConflict,
    entropyUnavailable,
    hashingUnavailable,
    storageError,
};

struct HumanAccountCreationRequest
{
    std::string actorId;
    std::string loginName;
    std::string displayName;
    std::string password;
    std::string idempotencyKey;
    std::string requestId;
    std::string correlationId;

    void clearSecrets() noexcept;
};

struct HumanAccountCreationResult
{
    HumanAccountCreationStatus status =
        HumanAccountCreationStatus::storageError;
    HumanAccountRecord account;
};

class HumanAccountCreationService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    HumanAccountCreationService(
        Database& database,
        SecurityIdentityProvisioningRepository& identityProvisioningRepository,
        HumanAccountRepository& humanAccountRepository,
        HumanAccountCreationRepository& creationRepository,
        CredentialVerifierRepository& credentialVerifierRepository,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    HumanAccountCreationResult create(
        HumanAccountCreationRequest request);

private:
    Database& database_;
    SecurityIdentityProvisioningRepository& identityProvisioningRepository_;
    HumanAccountRepository& humanAccountRepository_;
    HumanAccountCreationRepository& creationRepository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
