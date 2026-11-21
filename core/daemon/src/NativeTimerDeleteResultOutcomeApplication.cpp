#include "NativeTimerDeleteResultOutcomeApplication.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerDelete.h"
#include "BackendAgentNativeTimerDeleteLocalState.h"
#include "BackendAgentNativeTimerDeletePayload.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::daemon
{
namespace
{
using namespace vdrsuite::agent;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;
using Status = NativeTimerDeleteResultOutcomeStatus;

NativeTimerDeleteResultOutcomeResult result(
    Status status,
    const std::string& reason,
    const NativeTimerDeleteDispatchOutcomeResult& dispatch = {})
{
    NativeTimerDeleteResultOutcomeResult value;
    value.status = status;
    value.reasonCode = reason;
    value.dispatch = dispatch;
    return value;
}

bool revisionNext(
    const std::string& text, std::string& next)
{
    if (text.empty() || text.size() > 18U) return false;
    std::uint64_t value = 0;
    for (unsigned char ch : text)
    {
        if (ch < '0' || ch > '9') return false;
        const auto digit = static_cast<std::uint64_t>(ch - '0');
        if (value > (static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max() - 1U) - digit) / 10U)
            return false;
        value = value * 10U + digit;
    }
    if (value == 0 || value >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
        return false;
    next = std::to_string(value + 1U);
    return true;
}

bool expectedOutcome(
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    const BackendAgentCommandResult& generic)
{
    if (generic.completedAt != evidence.completedAt ||
        generic.commandId != evidence.commandId ||
        generic.requestFingerprint != evidence.requestFingerprint)
        return false;
    switch (evidence.outcome)
    {
        case BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect:
            return generic.dispatchState == "not_started" &&
                generic.verificationState == "verified" &&
                generic.resultCategory == "rejected";
        case BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified:
            return generic.dispatchState == "accepted_by_executor" &&
                generic.verificationState == "outcome_unknown" &&
                generic.resultCategory == "outcome_unknown";
        case BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown:
            return generic.dispatchState == "starting" &&
                generic.verificationState == "outcome_unknown" &&
                generic.resultCategory == "outcome_unknown";
    }
    return false;
}

NativeTimerDeleteExecutorOutcomeCategory executorCategory(
    BackendAgentNativeTimerDeleteOutcomeCategory value)
{
    switch (value)
    {
        case BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect:
            return NativeTimerDeleteExecutorOutcomeCategory::rejectedWithoutEffect;
        case BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified:
            return NativeTimerDeleteExecutorOutcomeCategory::acceptedUnverified;
        case BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown:
            return NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown;
    }
    return NativeTimerDeleteExecutorOutcomeCategory::outcomeUnknown;
}
}

NativeTimerDeleteResultOutcomeResult applyDurableNativeTimerDeleteResult(
    BackendAgentCommandRepository& commandRepository,
    MutationOperationRepository& operationRepository,
    NativeTimerBindingRepository& bindingRepository,
    NativeTimerDeleteDispatchService& dispatchService,
    const std::string& commandId)
{
    const auto assignment = commandRepository.findAssignment(commandId);
    if (!assignment ||
        assignment->commandType != kBackendAgentNativeTimerDeleteCommandType)
        return result(Status::missingCommand, "native_delete_command_missing");

    const auto durable = commandRepository.resultForCommand(commandId);
    if (!durable)
        return result(Status::missingResult, "native_delete_result_missing");

    BackendAgentNativeTimerDeleteEvidence evidence;
    std::string reason;
    if (durable->resultEvidence.empty() ||
        !backendAgentNativeTimerDeleteParseResultEvidence(
            durable->resultEvidence, *assignment, evidence, reason) ||
        !expectedOutcome(evidence, *durable))
        return result(Status::invalidEvidence, "native_delete_result_invalid");

    BackendAgentNativeTimerDeletePayload payload;
    if (!backendAgentNativeTimerDeleteParsePayload(
            assignment->payload, payload, reason))
        return result(Status::invalidEvidence, "native_delete_payload_invalid");

    const auto operation = operationRepository.findById(evidence.operationId);
    if (!operation.ok())
        return result(Status::unavailable, "native_delete_operation_unavailable");
    const auto binding = bindingRepository.findById(
        payload.nativeTimerBindingId);
    if (!binding.ok())
        return result(Status::unavailable, "native_delete_binding_unavailable");

    const auto& op = operation.operation;
    const auto& b = binding.binding;
    if (op.actionFamily != "timer.delete" ||
        op.resourceType != "NativeTimerBinding" ||
        op.resourceId != b.nativeTimerBindingId ||
        (op.expectedRevision != b.bindingRevision &&
         !(b.lastVerifiedOperationId == op.operationId &&
           b.missingSince > 0)) ||
        op.expectedResourceFingerprint != b.observedFingerprint ||
        op.backendId != b.backendId ||
        op.backendGeneration != b.backendGeneration ||
        op.backendId != assignment->backendId ||
        op.backendGeneration != assignment->backendGeneration ||
        payload.timerAssignmentId != b.timerAssignmentId ||
        payload.backendNativeTimerId != b.backendNativeTimerId ||
        payload.expectedBindingRevision != op.expectedRevision ||
        payload.operationRevision != evidence.operationRevision ||
        payload.controlPlaneClaimedAt <= 0 ||
        payload.controlPlaneClaimedAt > evidence.localStartingPersistedAt ||
        (b.missingSince != 0 && b.lastVerifiedOperationId != op.operationId) ||
        (b.driftState != NativeTimerBindingDriftState::none &&
         !(b.lastVerifiedOperationId == op.operationId &&
           b.driftState == NativeTimerBindingDriftState::expectedTransition)) ||
        (b.ownership != NativeTimerBindingOwnership::managed &&
         b.ownership != NativeTimerBindingOwnership::adopted))
        return result(Status::identityConflict, "native_delete_identity_conflict");

    std::string claimRevision;
    if (!revisionNext(evidence.operationRevision, claimRevision))
        return result(Status::revisionConflict, "native_delete_revision_invalid");

    // The durable Agent assignment is the authoritative claimedAt source,
    // including after daemon restart; never reinterpret current wall clock as
    // the start of an already-dispatched native operation.
    if (payload.controlPlaneClaimedAt != assignment->assignedAt)
        return result(Status::identityConflict, "native_delete_claim_time_conflict");

    NativeTimerDeleteDispatchClaim claim;
    claim.operationId = op.operationId;
    claim.operationRevision = claimRevision;
    claim.nativeTimerBindingId = b.nativeTimerBindingId;
    claim.expectedBindingRevision = op.expectedRevision;
    claim.expectedNativeTimerFingerprint = op.expectedResourceFingerprint;
    claim.backendId = b.backendId;
    claim.backendGeneration = b.backendGeneration;
    claim.backendNativeTimerId = b.backendNativeTimerId;
    claim.claimedAt = assignment->assignedAt;

    NativeTimerDeleteExecutorOutcome outcome;
    outcome.category = executorCategory(evidence.outcome);
    outcome.dispatchStartedAt = evidence.dispatchStartedAt;
    outcome.completedAt = evidence.completedAt;
    outcome.evidenceReference = evidence.evidenceReference;

    const auto applied = dispatchService.applyOutcome(claim, outcome);
    if (applied.status == NativeTimerDeleteDispatchOutcomeStatus::applied)
        return result(Status::applied, "native_delete_outcome_applied", applied);
    if (applied.status == NativeTimerDeleteDispatchOutcomeStatus::alreadyApplied)
        return result(Status::alreadyApplied, "native_delete_outcome_replayed", applied);
    if (applied.status == NativeTimerDeleteDispatchOutcomeStatus::operationRevisionConflict)
        return result(Status::revisionConflict, "native_delete_revision_conflict");
    return result(Status::operationConflict, "native_delete_outcome_conflict");
}
} // namespace vdrsuite::daemon
