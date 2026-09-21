#include "BackendAgentNativeTimerCreateLocalState.h"

#include "BackendAgentCommand.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace vdrsuite::agent
{
namespace
{
bool sameContext(
    const BackendAgentNativeTimerCreateCommand& command,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration)
{
    return command.backendId == backendId &&
        command.agentId == agentId &&
        command.agentInstanceId == agentInstanceId &&
        command.backendGeneration == backendGeneration;
}

BackendAgentNativeTimerCreateEvidence recoveryEvidence(
    const BackendAgentNativeTimerCreateLocalState& state,
    std::int64_t completedAt)
{
    const auto& command = state.command;
    BackendAgentNativeTimerCreateEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.timerAssignmentId = command.timerAssignmentId;
    evidence.nativeTimerBindingId = command.nativeTimerBindingId;
    evidence.jobId = command.jobId;
    evidence.attemptId = command.attemptId;
    evidence.claimEpoch = command.claimEpoch;
    evidence.backendId = command.backendId;
    evidence.agentId = command.agentId;
    evidence.agentInstanceId = command.agentInstanceId;
    evidence.backendGeneration = command.backendGeneration;
    evidence.providerInstanceEpoch = command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = state.localStartingPersistedAt;
    evidence.outcome = BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown;
    evidence.dispatchStartedAt = state.localStartingPersistedAt;
    evidence.completedAt = completedAt;
    evidence.evidenceReference = "local-recovery:" + command.commandId;
    return evidence;
}
}

BackendAgentNativeTimerCreateRecoveryResult
backendAgentNativeTimerCreateRecoverLocalState(
    const BackendAgentNativeTimerCreateLocalState& state,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now)
{
    BackendAgentNativeTimerCreateRecoveryResult result;
    std::string reason;
    if (!backendAgentNativeTimerCreateLocalStateValid(state, reason))
    {
        result.reasonCode = "native_timer_create_recovery_state_invalid";
        return result;
    }
    if (!backendAgentCommandSafeIdentifier(backendId) ||
        !backendAgentCommandSafeIdentifier(agentId) ||
        !backendAgentCommandSafeIdentifier(agentInstanceId) ||
        backendGeneration == 0 || now <= 0)
    {
        result.reasonCode = "native_timer_create_recovery_context_invalid";
        return result;
    }

    const bool contextCurrent = sameContext(
        state.command,
        backendId,
        agentId,
        agentInstanceId,
        backendGeneration);

    if (state.phase == BackendAgentNativeTimerCreateLocalPhase::completed)
    {
        result.decision =
            BackendAgentNativeTimerCreateRecoveryDecision::returnPersistedEvidence;
        result.evidence = state.evidence;
        result.reasonCode = contextCurrent
            ? "native_timer_create_completed_evidence_replay"
            : "native_timer_create_completed_evidence_survives_context_drift";
        return result;
    }

    // A durable starting record is the no-blind-retry boundary. The process may
    // have crashed immediately before or after native CREATE. Recovery therefore
    // treats the durable starting timestamp as the earliest possible dispatch and
    // emits outcome_unknown evidence for authoritative readback only.
    result.evidence = recoveryEvidence(
        state, std::max(now, state.localStartingPersistedAt));
    if (!backendAgentNativeTimerCreateEvidenceMatches(
            result.evidence, state.command, reason))
    {
        result.reasonCode = "native_timer_create_recovery_evidence_invalid";
        return result;
    }

    result.decision = BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly;
    result.reasonCode = contextCurrent
        ? "native_timer_create_starting_recovery_reconcile_only"
        : "native_timer_create_starting_context_fenced_reconcile_only";
    return result;
}

} // namespace vdrsuite::agent
