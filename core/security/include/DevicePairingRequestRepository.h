#pragma once

#include <string>

class Database;

enum class DevicePairingRequestRepositoryStatus
{
    ok,
    invalid,
    notFound,
    conflict,
    expired,
    invalidated,
    storageError,
    transactionRequired,
};

struct DevicePairingRequestRegistration
{
    std::string pairingRequestId;
    std::string userCodeHash;
    std::string pairingTokenHash;
    std::string displayName;
    std::string clientKind;
    std::string appVersion;
    std::string expiresAt;
};

struct StoredDevicePairingRequest
{
    std::string pairingRequestId;
    std::string userCodeHash;
    std::string pairingTokenHash;
    std::string displayName;
    std::string clientKind;
    std::string appVersion;
    std::string state;
    std::string expiresAt;
    std::string createdAt;
    bool expired = false;
    bool invalidated = false;
};

struct DevicePairingRequestLookupResult
{
    DevicePairingRequestRepositoryStatus status =
        DevicePairingRequestRepositoryStatus::storageError;
    StoredDevicePairingRequest request;
};

class DevicePairingRequestRepository
{
public:
    explicit DevicePairingRequestRepository(Database& database);

    bool ensureSchema();

    DevicePairingRequestRepositoryStatus registerInActiveTransaction(
        const DevicePairingRequestRegistration& registration);

    DevicePairingRequestLookupResult findById(
        const std::string& pairingRequestId) const;

    static bool supportsSecretHash(const std::string& secretHash);

private:
    Database& database_;
};
