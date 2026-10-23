#include "NativeTimerModifyResultOutcomeApplication.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerModify.h"
#include "BackendAgentNativeTimerModifyLocalState.h"
#include "BackendAgentNativeTimerModifyPayload.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
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
using S = NativeTimerModifyResultStatus;

NativeTimerModifyResultApplication result(
    S status, const NativeTimerModifyDispatchOutcomeResult& outcome = {})
{
    NativeTimerModifyResultApplication r;
    r.status = status;
    r.outcome = outcome;
    return r;
}

bool nextRevision(const std::string& value, std::string& next)
{
    if (value.empty() || value.size() > 18 || value.front() == '0') return false;
    std::uint64_t v = 0;
    for (unsigned char c : value) {
        if (c < '0' || c > '9') return false;
        const std::uint64_t d = c - '0';
        const auto max = static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max() - 1);
        if (v > (max - d) / 10) return false;
        v = v * 10 + d;
    }
    if (v == 0) return false;
    next = std::to_string(v + 1);
    return true;
}

bool resultMatches(
    const BackendAgentNativeTimerModifyEvidence& e,
    const BackendAgentCommandResult& generic)
{
    if (generic.commandId != e.commandId ||
        generic.requestFingerprint != e.requestFingerprint ||
        generic.completedAt != e.completedAt)
        return false;
    switch (e.outcome) {
        case BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect:
            return generic.dispatchState == "not_started" &&
                generic.resultCategory == "rejected" &&
                generic.verificationState == "verified";
        case BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified:
            return generic.dispatchState == "accepted_by_executor" &&
                generic.resultCategory == "outcome_unknown" &&
                generic.verificationState == "outcome_unknown";
        case BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown:
            return generic.dispatchState == "starting" &&
                generic.resultCategory == "outcome_unknown" &&
                generic.verificationState == "outcome_unknown";
    }
    return false;
}

NativeTimerModifyExecutorOutcomeCategory category(
    BackendAgentNativeTimerModifyOutcomeCategory outcome)
{
    switch (outcome) {
        case BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect:
            return NativeTimerModifyExecutorOutcomeCategory::rejectedWithoutEffect;
        case BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified:
            return NativeTimerModifyExecutorOutcomeCategory::acceptedUnverified;
        case BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown:
            return NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown;
    }
    return NativeTimerModifyExecutorOutcomeCategory::outcomeUnknown;
}

bool sameSpecification(
    const NativeTimerSpecification& a,
    const BackendAgentNativeTimerCreateSpecification& b)
{
    return a.channelId == b.channelId && a.title == b.title &&
        a.directory == b.directory && a.day == b.day &&
        a.weekdays == b.weekdays && a.startTime == b.startTime &&
        a.endTime == b.endTime && a.priority == b.priority &&
        a.lifetime == b.lifetime && a.enabled == b.enabled && a.vps == b.vps;
}
}

NativeTimerModifyResultApplication applyDurableNativeTimerModifyResult(
    BackendAgentCommandRepository& commands,
    MutationOperationRepository& operations,
    NativeTimerBindingRepository& bindings,
    NativeTimerModifyDispatchService& dispatcher,
    const std::string& commandId)
{
    const auto assignment = commands.findAssignment(commandId);
    if (!assignment ||
        (assignment->commandType != kBackendAgentNativeTimerUpdateCommandType &&
         assignment->commandType != kBackendAgentNativeTimerToggleCommandType))
        return result(S::unavailable);

    const auto durable = commands.resultForCommand(commandId);
    if (!durable) return result(S::missingResult);

    BackendAgentNativeTimerModifyEvidence evidence;
    std::string reason;
    if (durable->resultEvidence.empty() ||
        !backendAgentNativeTimerModifyParseResultEvidence(
            durable->resultEvidence, *assignment, evidence, reason) ||
        !resultMatches(evidence, *durable))
        return result(S::invalidEvidence);

    BackendAgentNativeTimerModifyPayload agentPayload;
    if (!backendAgentNativeTimerModifyParsePayload(
            assignment->payload, agentPayload, reason))
        return result(S::invalidEvidence);

    const auto found = operations.findById(evidence.operationId);
    const auto durablePayload =
        operations.findPayloadByOperationId(evidence.operationId);
    if (!found.ok() || !durablePayload.ok() ||
        durablePayload.payload.payloadType != "native.timer.modify" ||
        durablePayload.payload.payloadVersion != 1)
        return result(S::unavailable);

    NativeTimerModifyOperationPayload payload;
    if (!parseNativeTimerModifyOperationPayload(
            durablePayload.payload.payload, payload) ||
        nativeTimerModifyOperationPayloadFingerprint(payload) !=
            durablePayload.payload.payloadFingerprint)
        return result(S::invalidEvidence);

    const auto binding = bindings.findById(payload.nativeTimerBindingId);
    if (!binding.ok()) return result(S::unavailable);
    const auto& op = found.operation;
    const auto& b = binding.binding;

    if (op.actionFamily !=
            (payload.kind == NativeTimerModifyKind::update
                ? "timer.update" : "timer.toggle") ||
        op.resourceType != "NativeTimerBinding" ||
        op.resourceId != payload.nativeTimerBindingId ||
        op.expectedRevision != payload.expectedBindingRevision ||
        op.expectedResourceFingerprint != payload.expectedCurrentFingerprint ||
        op.backendId != payload.backendId ||
        op.backendGeneration != payload.backendGeneration ||
        op.backendId != assignment->backendId ||
        op.backendGeneration != assignment->backendGeneration ||
        b.backendId != payload.backendId ||
        b.backendGeneration != payload.backendGeneration ||
        b.backendNativeTimerId != payload.backendNativeTimerId ||
        b.timerAssignmentId != payload.timerAssignmentId ||
        (b.bindingRevision != payload.expectedBindingRevision &&
         b.lastVerifiedOperationId != op.operationId) ||
        (b.observedFingerprint != payload.expectedCurrentFingerprint &&
         b.lastVerifiedOperationId != op.operationId) ||
        (b.ownership != NativeTimerBindingOwnership::managed &&
         b.ownership != NativeTimerBindingOwnership::adopted) ||
        agentPayload.operationRevision != evidence.operationRevision ||
        agentPayload.timerAssignmentId != payload.timerAssignmentId ||
        agentPayload.expectedAssignmentRevision != payload.expectedAssignmentRevision ||
        agentPayload.expectedIntentRevision != payload.expectedIntentRevision ||
        agentPayload.assignmentEpoch != payload.assignmentEpoch ||
        agentPayload.nativeTimerBindingId != payload.nativeTimerBindingId ||
        agentPayload.expectedBindingRevision != payload.expectedBindingRevision ||
        agentPayload.backendId != payload.backendId ||
        agentPayload.backendGeneration != payload.backendGeneration ||
        agentPayload.backendNativeTimerId != payload.backendNativeTimerId ||
        agentPayload.expectedCurrentFingerprint !=
            backendAgentNativeTimerDeleteFingerprintToken(
                payload.expectedCurrentFingerprint) ||
        !sameSpecification(payload.expectedSpecification, agentPayload.specification) ||
        agentPayload.controlPlaneClaimedAt != assignment->assignedAt ||
        assignment->assignedAt > evidence.localStartingPersistedAt)
        return result(S::identityConflict);

    const bool shouldToggle = payload.kind == NativeTimerModifyKind::toggle;
    if ((assignment->commandType == kBackendAgentNativeTimerToggleCommandType)
        != shouldToggle)
        return result(S::identityConflict);

    std::string claimedRevision;
    if (!nextRevision(evidence.operationRevision, claimedRevision))
        return result(S::revisionConflict);
    NativeTimerModifyDispatchClaim claim;
    claim.operationId = op.operationId;
    claim.operationRevision = claimedRevision;
    claim.payload = payload;
    claim.claimedAt = assignment->assignedAt;

    NativeTimerModifyExecutorOutcome outcome;
    outcome.category = category(evidence.outcome);
    outcome.dispatchStartedAt = evidence.dispatchStartedAt;
    outcome.completedAt = evidence.completedAt;
    outcome.evidenceReference = evidence.evidenceReference;
    const auto applied = dispatcher.applyOutcome(claim, outcome);
    if (applied.status == NativeTimerModifyDispatchOutcomeStatus::applied)
        return result(S::applied, applied);
    if (applied.status == NativeTimerModifyDispatchOutcomeStatus::alreadyApplied)
        return result(S::alreadyApplied, applied);
    if (applied.status ==
        NativeTimerModifyDispatchOutcomeStatus::operationRevisionConflict)
        return result(S::revisionConflict);
    return result(S::operationConflict);
}
}
