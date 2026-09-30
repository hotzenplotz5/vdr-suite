#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>

class AccountabilityEventRepository;
class CredentialVerifierRepository;
class Database;
class FirstAdminBootstrapRepository;
class HumanAccountRepository;
class SecurityIdentityProvisioningRepository;
class SecurityPermissionGrantRepository;

enum class FirstAdminClaimStatus
{
    success,
    invalidRequest,
    claimed,
    bootstrapNotFound,
    bootstrapExpired,
    bootstrapConsumed,
    bootstrapInvalidated,
    bootstrapRejected,
    conflict,
    entropyUnavailable,
    hashingUnavailable,
    storageError,
};

struct FirstAdminClaimRequest
{
    std::string bootstrapId;
    std::string setupSecret;
    std::string loginName;
    std::string password;
    std::string displayName;
    std::string requestId;
    std::string correlationId;

    void clearSecrets() noexcept;
};

struct FirstAdminClaimResult
{
    FirstAdminClaimStatus status =
        FirstAdminClaimStatus::storageError;
    std::string accountId;
    std::string actorId;
    std::string credentialId;
};

class FirstAdminClaimService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    FirstAdminClaimService(
        Database& database,
        FirstAdminBootstrapRepository& bootstrapRepository,
        SecurityIdentityProvisioningRepository& identityProvisioningRepository,
        HumanAccountRepository& humanAccountRepository,
        CredentialVerifierRepository& credentialVerifierRepository,
        SecurityPermissionGrantRepository& permissionGrantRepository,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    FirstAdminClaimResult claim(FirstAdminClaimRequest request);

private:
    Database& database_;
    FirstAdminBootstrapRepository& bootstrapRepository_;
    SecurityIdentityProvisioningRepository& identityProvisioningRepository_;
    HumanAccountRepository& humanAccountRepository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    SecurityPermissionGrantRepository& permissionGrantRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
