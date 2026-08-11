#pragma once

#include "MutationOperation.h"

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

class MutationOperationRepository
{
public:
    explicit MutationOperationRepository(Database& database);

    bool ensureSchema();

    // Reserve one accepted ADR-0042 operation and its idempotency scope.
    // The repository owns the first durable operationRevision ("1").
    MutationOperationRepositoryResult reserve(
        const MutationOperation& operation);

    MutationOperationRepositoryResult findById(
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
