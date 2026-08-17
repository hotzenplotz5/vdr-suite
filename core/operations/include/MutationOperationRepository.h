#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>

class Database;

namespace vdrsuite::operations
{

enum class MutationOperationRepositoryStatus
{
    ok,
    invalid,
    notFound,
    idempotentReplay,
    idempotencyConflict,
    operationConflict,
    revisionConflict,
    stateConflict,
    storageError,
};

struct MutationOperationRepositoryResult
{
    MutationOperationRepositoryStatus status =
        MutationOperationRepositoryStatus::storageError;
    MutationOperation operation;

    bool ok() const
    {
        return status == MutationOperationRepositoryStatus::ok;
    }
};

// Immutable versioned handler input that may outlive the process which
// reserved the operation. The repository never interprets payload contents;
// domain services own serialization and validation of their payload type.
struct MutationOperationPayload
{
    std::string operationId;
    std::string payloadType;
    std::uint32_t payloadVersion = 0;
    std::string payload;
    std::string payloadFingerprint;
};

struct MutationOperationPayloadRepositoryResult
{
    MutationOperationRepositoryStatus status =
        MutationOperationRepositoryStatus::storageError;
    MutationOperationPayload payload;

    bool ok() const
    {
        return status == MutationOperationRepositoryStatus::ok;
    }
};

class MutationOperationRepository
{
public:
    explicit MutationOperationRepository(Database& database);

    bool ensureSchema();

    // Reserve one accepted ADR-0042 operation and its idempotency scope.
    // The repository owns the first durable operationRevision ("1").
    MutationOperationRepositoryResult reserve(
        const MutationOperation& operation);

    // Atomically reserve the operation and one immutable versioned payload in
    // the same SQLite transaction. Exact replay requires both the logical
    // operation and payload to match; a missing or changed payload fails closed
    // as operationConflict. Existing reserve() callers remain payload-free.
    MutationOperationRepositoryResult reserveWithPayload(
        const MutationOperation& operation,
        const MutationOperationPayload& payload);

    MutationOperationRepositoryResult findById(
        const std::string& operationId);

    MutationOperationPayloadRepositoryResult findPayloadByOperationId(
        const std::string& operationId);

    MutationOperationRepositoryResult findByIdempotencyScope(
        const std::string& actorId,
        const std::string& backendId,
        const std::string& resourceType,
        const std::string& resourceId,
        const std::string& actionFamily,
        const std::string& idempotencyKey);

    // State changes are revision-fenced and use the one shared ADR-0042
    // lifecycle. Domain services may correlate evidence to this authority but
    // must not maintain a second lifecycle state of their own.
    MutationOperationRepositoryResult transition(
        const std::string& operationId,
        const std::string& expectedOperationRevision,
        MutationOperationState expectedState,
        MutationOperationState nextState,
        const std::string& resultReference,
        std::int64_t updatedAt);

private:
    Database& database_;
};

}
