#pragma once

#include "MutationOperation.h"

#include <string>

namespace vdrsuite::operations
{

class MutationOperationRepository;

enum class MutationOperationReadStatus
{
    ok,
    invalid,
    notFound,
    storageError,
};

struct MutationOperationReadResult
{
    MutationOperationReadStatus status =
        MutationOperationReadStatus::storageError;
    MutationOperation operation;

    bool ok() const
    {
        return status == MutationOperationReadStatus::ok;
    }
};

// Read-only facade for public/application consumers.
//
// The repository remains the single ADR-0042 lifecycle authority. This facade
// never mutates operation state and deliberately hides an operation owned by a
// different actor as notFound so callers cannot use operation IDs as an
// existence oracle.
class MutationOperationReadService
{
public:
    explicit MutationOperationReadService(
        MutationOperationRepository& repository);

    MutationOperationReadResult findForActor(
        const std::string& operationId,
        const std::string& actorId) const;

private:
    MutationOperationRepository& repository_;
};

}
