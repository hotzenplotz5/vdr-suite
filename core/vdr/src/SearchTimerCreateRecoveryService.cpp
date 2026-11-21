#include "SearchTimerCreateRecoveryService.h"

#include "MutationOperationRepository.h"

#include <cstdint>
#include <limits>
#include <string>

namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;

SearchTimerCreateRecoveryResult result(
    SearchTimerCreateRecoveryStatus status,
    const MutationOperation& operation = {})
{
    SearchTimerCreateRecoveryResult output;
    output.status = status;
    output.operation = operation;
    return output;
}

bool exactCreate(const MutationOperation& operation,
    const SearchTimerCreateRecoveryRequest& request)
{
    return operation.operationId == request.operationId &&
        operation.actorId == request.actorId &&
        operation.backendId == request.backendId &&
        operation.backendGeneration == request.backendGeneration &&
        operation.resourceType == "SearchTimer" &&
        operation.resourceId == "new" &&
        operation.expectedRevision == "new" &&
        operation.actionFamily == "searchtimer.create" &&
        operation.verificationPolicy ==
            vdrsuite::operations::MutationOperationVerificationPolicy::
                readbackRequired;
}
}

SearchTimerCreateRecoveryService::SearchTimerCreateRecoveryService(
    vdrsuite::operations::MutationOperationRepository& operations)
    : operations_(operations)
{
}

SearchTimerCreateRecoveryResult
SearchTimerCreateRecoveryService::markOrphanedUnknown(
    const SearchTimerCreateRecoveryRequest& request)
{
    if (!request.dispatcherStopped || request.operationId.empty() ||
        request.actorId.empty() || request.backendId.empty() ||
        request.backendGeneration == 0U ||
        request.backendGeneration > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        request.now <= 0 || request.staleAfterSeconds < 60 ||
        request.staleAfterSeconds > 86400)
        return result(SearchTimerCreateRecoveryStatus::denied);

    const auto found = operations_.findById(request.operationId);
    if (found.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateRecoveryStatus::unavailable);

    const MutationOperation& operation = found.operation;
    if (!exactCreate(operation, request))
        return result(SearchTimerCreateRecoveryStatus::denied, operation);

    if (operation.state == MutationOperationState::outcomeUnknown)
        return result(SearchTimerCreateRecoveryStatus::alreadyUnknown,
            operation);

    // Not even a stale accepted operation may become dispatching or be
    // redispatched through this read/recovery path.
    if (operation.state != MutationOperationState::dispatching &&
        operation.state != MutationOperationState::executedUnverified)
        return result(SearchTimerCreateRecoveryStatus::conflict, operation);

    if (request.now < operation.updatedAt ||
        request.now - operation.updatedAt < request.staleAfterSeconds)
        return result(SearchTimerCreateRecoveryStatus::notExpired, operation);

    // Revision-fenced durable transition; if a live worker advanced in the
    // meantime this fails closed, without a second backend CREATE attempt.
    // Retain any already-recorded resultReference rather than discarding
    // possible future reconciliation evidence.
    const auto changed = operations_.transition(
        operation.operationId, operation.operationRevision, operation.state,
        MutationOperationState::outcomeUnknown,
        operation.resultReference, request.now);
    if (changed.status == MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateRecoveryStatus::markedUnknown,
            changed.operation);
    if (changed.status == MutationOperationRepositoryStatus::revisionConflict ||
        changed.status == MutationOperationRepositoryStatus::stateConflict ||
        changed.status == MutationOperationRepositoryStatus::idempotentReplay)
        return result(SearchTimerCreateRecoveryStatus::conflict,
            changed.operation);
    return result(SearchTimerCreateRecoveryStatus::unavailable);
}
