#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Database;

enum class DevicePairingRequestRepositoryStatus
{
    ok,
    invalid,
    notFound,
    conflict,
    revisionConflict,
    stateConflict,
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
    std::uint64_t revision = 0;
    std::string decidedByActorId;
    std::string decidedAt;
    bool expired = false;
    bool invalidated = false;
};

struct DevicePairingRequestLookupResult
{
    DevicePairingRequestRepositoryStatus status =
        DevicePairingRequestRepositoryStatus::storageError;
    StoredDevicePairingRequest request;
};

struct DevicePairingRequestListResult
{
    DevicePairingRequestRepositoryStatus status =
        DevicePairingRequestRepositoryStatus::storageError;
    std::vector<StoredDevicePairingRequest> requests;
    bool hasMore = false;
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

    DevicePairingRequestListResult listPending(
        const std::string& afterPairingRequestId,
        std::size_t limit) const;

    DevicePairingRequestRepositoryStatus decideInActiveTransaction(
        const std::string& pairingRequestId,
        std::uint64_t expectedRevision,
        const std::string& state,
        const std::string& decidedByActorId);

    // Requires the caller's active issuance transaction. This is not an
    // authorization check: the service must verify the pairing token first.
    DevicePairingRequestRepositoryStatus consumeApprovedInActiveTransaction(
        const std::string& pairingRequestId,
        std::uint64_t expectedRevision);

    static bool supportsSecretHash(const std::string& secretHash);

private:
    Database& database_;
};
