#include "SearchTimerCreateDispatchService.h"

#include "ISearchTimerCommandExecutor.h"
#include "ISearchTimerDataSource.h"
#include "MutationOperationRepository.h"
#include "SearchTimerCreateOperationPayload.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerCreateDeviceAdmissionService.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>

namespace
{
using vdrsuite::operations::MutationOperation;
using vdrsuite::operations::MutationOperationRepositoryStatus;
using vdrsuite::operations::MutationOperationState;

SearchTimerCreateDispatchResult result(
    SearchTimerCreateDispatchStatus status,
    const MutationOperation& operation = {})
{
    SearchTimerCreateDispatchResult answer;
    answer.status = status;
    answer.operation = operation;
    return answer;
}

bool completeInventory(const SearchTimerResult& inventory)
{
    return inventory.available() && inventory.totalCount() >= 0 &&
        static_cast<std::size_t>(inventory.totalCount()) ==
            inventory.items().size();
}

bool matchesRequested(
    const SearchTimer& timer, const SearchTimerCreateRequest& request)
{
    return timer.backendId() == request.backendId &&
        !timer.backendNativeId().empty() &&
        timer.name() == request.name &&
        timer.query() == request.query &&
        timer.state() == (request.active
            ? SearchTimerState::Active : SearchTimerState::Inactive) &&
        timer.matchOptions().mode() == request.matchMode &&
        timer.matchOptions().tolerance() == request.matchTolerance &&
        timer.comparisonOptions().compareTitle() == request.compareTitle &&
        timer.comparisonOptions().compareSubtitle() ==
            request.compareSubtitle &&
        timer.comparisonOptions().compareSummary() == request.compareSummary;
}

SearchTimerCreateRequest commandFor(
    const MutationOperation& operation,
    const SearchTimerCreateOperationPayload& payload)
{
    SearchTimerCreateRequest command;
    command.backendId = operation.backendId;
    command.name = payload.name;
    command.query = payload.query;
    command.active = payload.active;
    command.matchMode = payload.matchMode;
    command.matchTolerance = payload.matchTolerance;
    command.compareTitle = payload.compareTitle;
    command.compareSubtitle = payload.compareSubtitle;
    command.compareSummary = payload.compareSummary;
    return command;
}

bool correctOperation(
    const MutationOperation& operation,
    const SearchTimerCreateDispatchRequest& request)
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

SearchTimerCreateDispatchService::SearchTimerCreateDispatchService(
    vdrsuite::operations::MutationOperationRepository& operations,
    const ISearchTimerBackendGenerationAuthority& generations)
    : operations_(operations), generations_(generations)
{
}

SearchTimerCreateDispatchResult SearchTimerCreateDispatchService::dispatchOnce(
    const SearchTimerCreateDispatchRequest& request,
    ISearchTimerCommandExecutor& executor,
    ISearchTimerDataSource& readback)
{
    using namespace vdrsuite::operations;

    if (request.operationId.empty() || request.actorId.empty() ||
        request.backendId.empty() || request.backendGeneration == 0U ||
        request.now <= 0 || !request.operatorConfirmed ||
        !request.backendWriteAllowed || !request.executorOptIn)
        return result(SearchTimerCreateDispatchStatus::denied);

    const auto found = operations_.findById(request.operationId);
    if (found.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateDispatchStatus::unavailable);
    const MutationOperation& operation = found.operation;
    if (!correctOperation(operation, request))
        return result(SearchTimerCreateDispatchStatus::denied, operation);
    if (operation.state != MutationOperationState::accepted)
        return result(SearchTimerCreateDispatchStatus::conflict, operation);
    if (request.now < operation.updatedAt)
        return result(SearchTimerCreateDispatchStatus::stale, operation);

    const auto stored = operations_.findPayloadByOperationId(
        operation.operationId);
    SearchTimerCreateOperationPayload payload;
    if (!stored.ok() || stored.payload.payloadType != "searchtimer.create" ||
        stored.payload.payloadVersion != 1U ||
        !parseSearchTimerCreateOperationPayload(
            stored.payload.payload, payload) ||
        stored.payload.payloadFingerprint !=
            searchTimerCreateOperationPayloadFingerprint(payload) ||
        stored.payload.payloadFingerprint != operation.requestFingerprint ||
        stored.payload.payloadFingerprint !=
            operation.expectedResourceFingerprint)
        return result(SearchTimerCreateDispatchStatus::unavailable, operation);

    // Previously admitted active CREATE operations can survive daemon
    // upgrades. Reject them durably BEFORE contacting the provider, so
    // the old operation cannot be left accepted for later execution.
    if (payload.active)
    {
        const auto blocked = operations_.transition(
            operation.operationId, operation.operationRevision,
            MutationOperationState::accepted,
            MutationOperationState::failedBeforeDispatch,
            "searchtimer-active-create-disallowed", request.now);
        return blocked.status == MutationOperationRepositoryStatus::ok
            ? result(SearchTimerCreateDispatchStatus::denied,
                blocked.operation)
            : result(SearchTimerCreateDispatchStatus::unavailable, operation);
    }

    // A retained accepted operation is not authority to send a POST to a
    // restarted backend. Validate live generation before the inventory read.
    try
    {
        const auto live = generations_.currentGeneration(operation.backendId);
        if (!live || *live != operation.backendGeneration)
            return result(SearchTimerCreateDispatchStatus::unavailable, operation);
    }
    catch (...)
    {
        return result(SearchTimerCreateDispatchStatus::unavailable, operation);
    }

    const SearchTimerCreateRequest command = commandFor(operation, payload);
    std::unordered_set<std::string> existingIds;
    // Before dispatch, get an authoritative, unpaged backend snapshot.
    // Without one an unlabelled provider response could be misattributed.
    try
    {
        const SearchTimerResult baseline =
            readback.list(SearchTimerQuery::byBackend(operation.backendId));
        if (!completeInventory(baseline))
            return result(SearchTimerCreateDispatchStatus::notReady, operation);
        for (const auto& timer : baseline.items())
        {
            if (timer.backendId() != operation.backendId ||
                timer.backendNativeId().empty() ||
                !existingIds.insert(timer.backendNativeId()).second)
                return result(SearchTimerCreateDispatchStatus::notReady,
                    operation);
        }
    }
    catch (...)
    {
        return result(SearchTimerCreateDispatchStatus::notReady, operation);
    }

    // The inventory fetch itself can span an Agent restart. Re-check the
    // server-owned generation immediately before consuming the one-shot CAS;
    // otherwise an accepted pre-restart operation could create on a successor.
    try
    {
        const auto live = generations_.currentGeneration(operation.backendId);
        if (!live || *live != operation.backendGeneration)
            return result(SearchTimerCreateDispatchStatus::unavailable, operation);
    }
    catch (...)
    {
        return result(SearchTimerCreateDispatchStatus::unavailable, operation);
    }

    // ONLY an exact successful CAS can cause the single backend CREATE.
    // Repository "idempotentReplay" MUST NOT lead to executor invocation.
    const auto locked = operations_.transition(
        operation.operationId, operation.operationRevision,
        MutationOperationState::accepted, MutationOperationState::dispatching,
        "", request.now);
    if (locked.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateDispatchStatus::conflict,
            locked.operation);

    const auto unknown = [this, &request](
        const MutationOperation& current) {
        const auto transitioned = operations_.transition(
            current.operationId, current.operationRevision,
            current.state, MutationOperationState::outcomeUnknown,
            "", request.now);
        return result(SearchTimerCreateDispatchStatus::outcomeUnknown,
            transitioned.status == MutationOperationRepositoryStatus::ok
                ? transitioned.operation : current);
    };

    SearchTimerCreateResult created;
    try { created = executor.create(command); }
    catch (...) { return unknown(locked.operation); }

    // A failure or missing/old backend ID after an attempted POST is UNKNOWN,
    // never "not created": it might have been applied before transport broke.
    if (!created.success ||
        created.searchTimer.backendNativeId().empty() ||
        existingIds.count(created.searchTimer.backendNativeId()) != 0U ||
        created.searchTimer.backendId() != operation.backendId ||
        created.searchTimer.name() != payload.name ||
        created.searchTimer.query() != payload.query ||
        created.searchTimer.isActive() != payload.active)
        return unknown(locked.operation);

    const auto unverified = operations_.transition(
        operation.operationId, locked.operation.operationRevision,
        MutationOperationState::dispatching,
        MutationOperationState::executedUnverified,
        "", request.now);
    if (unverified.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateDispatchStatus::outcomeUnknown,
            locked.operation);

    bool verified = false;
    try
    {
        const SearchTimerResult after =
            readback.list(SearchTimerQuery::byBackend(operation.backendId));
        if (completeInventory(after))
        {
            int nativeIdentityCount = 0;
            int exactMatches = 0;
            for (const auto& timer : after.items())
            {
                if (timer.backendNativeId() !=
                    created.searchTimer.backendNativeId())
                    continue;
                ++nativeIdentityCount;
                if (matchesRequested(timer, command))
                    ++exactMatches;
            }
            // A repeated native ID cannot be authoritative even if just
            // one of the duplicate rows matches the requested options.
            verified = nativeIdentityCount == 1 && exactMatches == 1;
        }
    }
    catch (...)
    {
        verified = false;
    }
    if (!verified) return unknown(unverified.operation);

    const auto completed = operations_.transition(
        operation.operationId, unverified.operation.operationRevision,
        MutationOperationState::executedUnverified,
        MutationOperationState::succeeded,
        "searchtimer-create-verified", request.now);
    if (completed.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateDispatchStatus::outcomeUnknown,
            unverified.operation);
    return result(SearchTimerCreateDispatchStatus::verified,
        completed.operation);
}
