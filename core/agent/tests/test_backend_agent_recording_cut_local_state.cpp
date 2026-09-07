#include "BackendAgentRecordingCutLocalState.h"
#include "BackendAgentRecordingCutPayload.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{
BackendAgentRecordingCutPayload payload()
{
    BackendAgentRecordingCutPayload value;
    value.operationRevision = "4";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 100;
    auto& selection = value.localProviderSelection;
    selection.backendId = value.backendId;
    selection.authorityDomain = kBackendAgentRecordingCutAuthorityDomain;
    selection.providerId = kBackendAgentRecordingCutProviderId;
    selection.providerKind = kBackendAgentRecordingCutProviderKind;
    selection.ownershipGeneration = 1;
    selection.providerInstanceEpoch = "pie_cut_1";
    selection.providerGeneration = 1;
    selection.capabilityRevision = 1;
    selection.requiredCapability = kBackendAgentRecordingCutCapability;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_cut_1";
    value.correlationId = "req_cut_1";
    value.operationId = "op_cut_1";
    value.commandType = kBackendAgentRecordingCutCommandType;
    value.payloadVersion = kBackendAgentRecordingCutPayloadVersion;
    value.payload = backendAgentRecordingCutPayload(payload());
    value.verificationPolicy = "readback_required";
    value.jobId = "job_cut_1";
    value.attemptId = "attempt_cut_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_cut_1";
    value.backendId = "default";
    value.agentId = "agent_cut_1";
    value.agentInstanceId = "instance_cut_1";
    value.backendGeneration = 7;
    value.assignedAt = 100;
    value.deadline = 1000;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    return value;
}
}

int main()
{
    const auto a = assignment();
    assert(backendAgentCommandValidAssignment(a));
    std::string reason;

    BackendAgentRecordingCutLocalState starting;
    assert(backendAgentRecordingCutPrepareLocalStarting(a, 101, starting, reason));
    assert(starting.phase == BackendAgentRecordingCutLocalPhase::starting);

    const std::string encoded =
        backendAgentRecordingCutSerializeLocalState(starting, reason);
    assert(!encoded.empty());
    BackendAgentRecordingCutLocalState parsed;
    assert(backendAgentRecordingCutParseLocalState(encoded, parsed, reason));

    const auto recovery = backendAgentRecordingCutRecoverLocalState(
        parsed, "default", "agent_cut_1", "instance_cut_1", 7, 102);
    assert(recovery.decision == BackendAgentRecordingCutRecoveryDecision::reconcileOnly);
    assert(recovery.evidence.outcome == BackendAgentRecordingCutOutcomeCategory::outcomeUnknown);
    assert(recovery.reasonCode == "recording_cut_starting_recovery_reconcile_only");

    const auto fenced = backendAgentRecordingCutRecoverLocalState(
        parsed, "default", "agent_cut_1", "replacement_instance", 7, 103);
    assert(fenced.decision == BackendAgentRecordingCutRecoveryDecision::reconcileOnly);
    assert(fenced.reasonCode == "recording_cut_starting_context_fenced_reconcile_only");

    assert(backendAgentRecordingCutCompleteLocalState(
        parsed, recovery.evidence, reason));
    const std::string completed =
        backendAgentRecordingCutSerializeLocalState(parsed, reason);
    BackendAgentRecordingCutLocalState completedParsed;
    assert(backendAgentRecordingCutParseLocalState(
        completed, completedParsed, reason));
    const auto replay = backendAgentRecordingCutRecoverLocalState(
        completedParsed, "default", "agent_cut_1", "instance_cut_1", 7, 110);
    assert(replay.decision ==
        BackendAgentRecordingCutRecoveryDecision::returnPersistedEvidence);
    assert(replay.evidence.outcome == BackendAgentRecordingCutOutcomeCategory::outcomeUnknown);
    return 0;
}
