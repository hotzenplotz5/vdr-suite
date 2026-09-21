#include "BackendAgentNativeTimerDeleteLocalState.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{

const std::string& timerFingerprint()
{
    static const std::string value =
        "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 11;
    selection.providerInstanceEpoch = "suitebridge-epoch-4";
    selection.providerGeneration = 7;
    selection.capabilityRevision = 3;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "op-rev-9";
    payload.nativeTimerBindingId = "binding-17";
    payload.expectedBindingRevision = "binding-rev-6";
    payload.expectedNativeTimerFingerprint = timerFingerprint();
    payload.timerAssignmentId = "timer-assignment-12";
    payload.backendNativeTimerId = "native-timer-44";
    payload.controlPlaneClaimedAt = 100;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-1";
    value.correlationId = "correlation-1";
    value.operationId = "operation-1";
    value.jobId = "job-1";
    value.attemptId = "attempt-1";
    value.claimEpoch = 2;
    value.commandId = "command-1";
    value.backendId = "default";
    value.agentId = "agent-1";
    value.agentInstanceId = "instance-1";
    value.backendGeneration = 23;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 110;
    value.deadline = 300;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentNativeTimerDeleteEvidence evidence(
    const BackendAgentNativeTimerDeleteLocalState& state,
    BackendAgentNativeTimerDeleteOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    std::int64_t completedAt,
    const std::string& reference)
{
    const auto& command = state.command;
    BackendAgentNativeTimerDeleteEvidence value;
    value.commandId = command.commandId;
    value.requestFingerprint = command.requestFingerprint;
    value.operationId = command.operationId;
    value.operationRevision = command.operationRevision;
    value.jobId = command.jobId;
    value.attemptId = command.attemptId;
    value.claimEpoch = command.claimEpoch;
    value.backendId = command.backendId;
    value.agentId = command.agentId;
    value.agentInstanceId = command.agentInstanceId;
    value.backendGeneration = command.backendGeneration;
    value.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    value.localStartingPersistedAt = state.localStartingPersistedAt;
    value.outcome = outcome;
    value.dispatchStartedAt = dispatchStartedAt;
    value.completedAt = completedAt;
    value.evidenceReference = reference;
    return value;
}

}

int main()
{
    const BackendAgentCommandAssignment assigned = assignment();
    std::string reason;

    BackendAgentNativeTimerDeleteCommand command;
    assert(backendAgentNativeTimerDeleteCommandFromAssignment(
        assigned, command, reason));
    assert(command.operationId == assigned.operationId);
    assert(command.operationRevision == "op-rev-9");
    assert(command.nativeTimerBindingId == "binding-17");
    assert(command.expectedNativeTimerFingerprint == timerFingerprint());
    assert(command.backendNativeTimerId == "native-timer-44");
    assert(command.localProviderSelection.providerInstanceEpoch ==
           "suitebridge-epoch-4");

    BackendAgentNativeTimerDeleteLocalState starting;
    assert(backendAgentNativeTimerDeletePrepareLocalStarting(
        assigned, 120, starting, reason));
    assert(reason == "native_timer_delete_starting_ready_for_durable_persist");
    assert(starting.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(starting.localStartingPersistedAt == 120);
    assert(backendAgentNativeTimerDeleteLocalStateValid(starting, reason));

    // The typed local state has no execution/retry decision. Once "starting" is
    // durable, every recovery path is reconciliation-only.
    const auto recovered = backendAgentNativeTimerDeleteRecoverLocalState(
        starting, "default", "agent-1", "instance-1", 23, 130);
    assert(recovered.decision ==
           BackendAgentNativeTimerDeleteRecoveryDecision::reconcileOnly);
    assert(recovered.reasonCode ==
           "native_timer_delete_starting_recovery_reconcile_only");
    assert(recovered.evidence.outcome ==
           BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown);
    assert(recovered.evidence.localStartingPersistedAt == 120);
    assert(recovered.evidence.dispatchStartedAt == 120);
    assert(recovered.evidence.completedAt == 130);
    assert(backendAgentNativeTimerDeleteEvidenceMatches(
        recovered.evidence, starting.command, reason));

    // Agent/backend generation drift must never reopen execution.
    const auto generationDrift = backendAgentNativeTimerDeleteRecoverLocalState(
        starting, "default", "agent-1", "instance-1", 24, 131);
    assert(generationDrift.decision ==
           BackendAgentNativeTimerDeleteRecoveryDecision::reconcileOnly);
    assert(generationDrift.reasonCode ==
           "native_timer_delete_starting_context_fenced_reconcile_only");
    assert(generationDrift.evidence.commandId == starting.command.commandId);

    // Clock rollback also remains safe: the conservative completion fence never
    // precedes durable local starting.
    const auto clockRollback = backendAgentNativeTimerDeleteRecoverLocalState(
        starting, "default", "agent-1", "instance-1", 23, 119);
    assert(clockRollback.decision ==
           BackendAgentNativeTimerDeleteRecoveryDecision::reconcileOnly);
    assert(clockRollback.evidence.completedAt == 120);

    const std::string encodedStarting =
        backendAgentNativeTimerDeleteSerializeLocalState(starting, reason);
    assert(!encodedStarting.empty());
    assert(encodedStarting.find(
               "expected_native_timer_fingerprint=" + timerFingerprint() + "\n") !=
           std::string::npos);
    BackendAgentNativeTimerDeleteLocalState parsedStarting;
    assert(backendAgentNativeTimerDeleteParseLocalState(
        encodedStarting, parsedStarting, reason));
    assert(parsedStarting.phase ==
           BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(parsedStarting.command.requestFingerprint ==
           starting.command.requestFingerprint);
    assert(parsedStarting.command.expectedNativeTimerFingerprint ==
           starting.command.expectedNativeTimerFingerprint);
    assert(parsedStarting.command.localProviderSelection.ownershipGeneration == 11);
    assert(parsedStarting.localStartingPersistedAt == 120);

    // A local pre-dispatch fence failure may prove rejected-without-effect.
    BackendAgentNativeTimerDeleteLocalState rejectedState = starting;
    const auto rejected = evidence(
        rejectedState,
        BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect,
        0,
        125,
        "local-provider-fence-rejected");
    assert(backendAgentNativeTimerDeleteCompleteLocalState(
        rejectedState, rejected, reason));
    assert(rejectedState.phase ==
           BackendAgentNativeTimerDeleteLocalPhase::completed);

    const auto rejectedReplay = backendAgentNativeTimerDeleteRecoverLocalState(
        rejectedState, "default", "agent-1", "instance-1", 99, 140);
    assert(rejectedReplay.decision ==
           BackendAgentNativeTimerDeleteRecoveryDecision::returnPersistedEvidence);
    assert(rejectedReplay.reasonCode ==
           "native_timer_delete_completed_evidence_survives_context_drift");
    assert(rejectedReplay.evidence.outcome ==
           BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect);

    // Dispatch-capable outcomes cannot claim a boundary before durable starting.
    BackendAgentNativeTimerDeleteLocalState acceptedState = starting;
    auto tooEarly = evidence(
        acceptedState,
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified,
        119,
        126,
        "executor-accepted");
    assert(!backendAgentNativeTimerDeleteCompleteLocalState(
        acceptedState, tooEarly, reason));
    assert(reason == "native_timer_delete_completion_evidence_mismatch");
    assert(acceptedState.phase ==
           BackendAgentNativeTimerDeleteLocalPhase::starting);

    auto accepted = evidence(
        acceptedState,
        BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified,
        121,
        126,
        "executor-accepted");
    assert(backendAgentNativeTimerDeleteCompleteLocalState(
        acceptedState, accepted, reason));
    const std::string encodedCompleted =
        backendAgentNativeTimerDeleteSerializeLocalState(acceptedState, reason);
    assert(!encodedCompleted.empty());

    BackendAgentNativeTimerDeleteLocalState parsedCompleted;
    assert(backendAgentNativeTimerDeleteParseLocalState(
        encodedCompleted, parsedCompleted, reason));
    assert(parsedCompleted.phase ==
           BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(parsedCompleted.command.expectedNativeTimerFingerprint ==
           timerFingerprint());
    assert(parsedCompleted.evidence.dispatchStartedAt == 121);
    assert(parsedCompleted.evidence.completedAt == 126);

    const auto persisted = backendAgentNativeTimerDeleteRecoverLocalState(
        parsedCompleted, "default", "agent-1", "instance-1", 23, 150);
    assert(persisted.decision ==
           BackendAgentNativeTimerDeleteRecoveryDecision::returnPersistedEvidence);
    assert(persisted.reasonCode ==
           "native_timer_delete_completed_evidence_replay");
    assert(persisted.evidence.evidenceReference == "executor-accepted");

    // Exact-state decoding fails closed when the immutable provider fence is
    // altered independently of the rest of the command envelope.
    std::string tampered = encodedStarting;
    const std::string provider = "provider_backend_id=default";
    const auto providerAt = tampered.find(provider);
    assert(providerAt != std::string::npos);
    tampered.replace(
        providerAt,
        provider.size(),
        "provider_backend_id=other");
    BackendAgentNativeTimerDeleteLocalState invalid;
    assert(!backendAgentNativeTimerDeleteParseLocalState(
        tampered, invalid, reason));

    std::string missingFingerprint = encodedStarting;
    const std::string fingerprint =
        "expected_native_timer_fingerprint=" + timerFingerprint();
    const auto fingerprintAt = missingFingerprint.find(fingerprint);
    assert(fingerprintAt != std::string::npos);
    missingFingerprint.replace(
        fingerprintAt,
        fingerprint.size(),
        "expected_native_timer_fingerprint=");
    assert(!backendAgentNativeTimerDeleteParseLocalState(
        missingFingerprint, invalid, reason));

    BackendAgentCommandAssignment wrongType = assigned;
    wrongType.commandType = "probe.noop";
    wrongType.payloadVersion = 1;
    wrongType.payload = "{}";
    wrongType.verificationPolicy = "none";
    wrongType.requestFingerprint = backendAgentCommandFingerprint(wrongType);
    assert(!backendAgentNativeTimerDeletePrepareLocalStarting(
        wrongType, 120, invalid, reason));

    assert(!backendAgentNativeTimerDeletePrepareLocalStarting(
        assigned, 301, invalid, reason));
    assert(reason == "native_timer_delete_starting_time_fenced");

    // There is intentionally no SuiteBridge/VDR mutation in Slice 27.
    return 0;
}
