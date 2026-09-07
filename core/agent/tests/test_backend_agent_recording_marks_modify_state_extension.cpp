#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentRecordingMarksModifyPayload.h"

#include <cassert>
#include <string>
#include <vector>

using namespace vdrsuite::agent;

namespace
{

BackendAgentCommandAssignment assignment()
{
    BackendAgentRecordingMarksModifyPayload payload;
    payload.kind = BackendAgentRecordingMarksModifyKind::replace;
    payload.operationRevision = "4";
    payload.recordingKey = "0123456789abcdef0123456789abcdef";
    payload.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    payload.replacementFrames = {10, 20, 30};
    payload.backendId = "default";
    payload.backendGeneration = 7;
    payload.controlPlaneClaimedAt = 100;
    payload.localProviderSelection.backendId = "default";
    payload.localProviderSelection.authorityDomain =
        kBackendAgentRecordingMarksModifyAuthorityDomain;
    payload.localProviderSelection.providerId =
        kBackendAgentRecordingMarksModifyProviderId;
    payload.localProviderSelection.providerKind =
        kBackendAgentRecordingMarksModifyProviderKind;
    payload.localProviderSelection.ownershipGeneration = 1;
    payload.localProviderSelection.providerInstanceEpoch = "pie_marks_1";
    payload.localProviderSelection.providerGeneration = 2;
    payload.localProviderSelection.capabilityRevision = 3;
    payload.localProviderSelection.requiredCapability =
        kBackendAgentRecordingMarksModifyCapability;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_marks_state_ext";
    value.correlationId = "corr_marks_state_ext";
    value.operationId = "op_marks_state_ext";
    value.jobId = "job_marks_state_ext";
    value.attemptId = "attempt_marks_state_ext";
    value.claimEpoch = 1;
    value.commandId = "cmd_marks_state_ext";
    value.backendId = "default";
    value.agentId = "agent_marks_state_ext";
    value.agentInstanceId = "instance_marks_state_ext";
    value.backendGeneration = 7;
    value.commandType = kBackendAgentRecordingMarksModifyCommandType;
    value.payloadVersion = kBackendAgentRecordingMarksModifyPayloadVersion;
    value.payload = backendAgentRecordingMarksModifyPayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 110;
    value.deadline = 500;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentRecordingMarksModifyEvidence evidenceFor(
    const BackendAgentRecordingMarksModifyLocalState& state)
{
    BackendAgentRecordingMarksModifyEvidence evidence;
    evidence.commandId = state.command.commandId;
    evidence.requestFingerprint = state.command.requestFingerprint;
    evidence.operationId = state.command.operationId;
    evidence.operationRevision = state.command.operationRevision;
    evidence.jobId = state.command.jobId;
    evidence.attemptId = state.command.attemptId;
    evidence.claimEpoch = state.command.claimEpoch;
    evidence.backendId = state.command.backendId;
    evidence.agentId = state.command.agentId;
    evidence.agentInstanceId = state.command.agentInstanceId;
    evidence.backendGeneration = state.command.backendGeneration;
    evidence.providerInstanceEpoch =
        state.command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = state.localStartingPersistedAt;
    evidence.outcome =
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown;
    evidence.dispatchStartedAt = 121;
    evidence.completedAt = 125;
    evidence.evidenceReference = "nmarks:state-extension:test";
    return evidence;
}

}

int main()
{
    const auto assigned = assignment();
    std::string reasonCode;

    BackendAgentRecordingMarksModifyLocalState starting;
    assert(backendAgentRecordingMarksModifyPrepareLocalStarting(
        assigned, 120, starting, reasonCode));

    const auto encoded = backendAgentRecordingMarksModifyCommandStateExtension(
        assigned, starting, reasonCode);
    assert(!encoded.empty());
    assert(encoded.rfind("cse1.", 0) == 0);
    assert(encoded.find('\n') == std::string::npos);

    BackendAgentCommandStateExtension generic;
    assert(backendAgentCommandStateExtensionParse(
        encoded, assigned, generic, reasonCode));
    assert(generic.extensionType ==
        kBackendAgentRecordingMarksModifyLocalStateExtensionType);
    assert(backendAgentCommandStateExtensionValidateSupported(
        generic, assigned, reasonCode));

    BackendAgentRecordingMarksModifyLocalState parsed;
    assert(backendAgentRecordingMarksModifyParseCommandStateExtension(
        encoded, assigned, parsed, reasonCode));
    assert(parsed.phase == BackendAgentRecordingMarksModifyLocalPhase::starting);
    assert(parsed.localStartingPersistedAt == 120);
    assert(parsed.command.replacementFrames == std::vector<int>({10, 20, 30}));

    auto different = assigned;
    different.operationId = "op_marks_state_other";
    different.requestFingerprint = backendAgentCommandFingerprint(different);
    assert(backendAgentCommandValidAssignment(different));
    assert(!backendAgentRecordingMarksModifyParseCommandStateExtension(
        encoded, different, parsed, reasonCode));

    auto completed = starting;
    const auto evidence = evidenceFor(starting);
    assert(backendAgentRecordingMarksModifyCompleteLocalState(
        completed, evidence, reasonCode));
    const auto completedEncoded =
        backendAgentRecordingMarksModifyCommandStateExtension(
            assigned, completed, reasonCode);
    assert(!completedEncoded.empty());
    assert(backendAgentRecordingMarksModifyParseCommandStateExtension(
        completedEncoded, assigned, parsed, reasonCode));
    assert(parsed.phase == BackendAgentRecordingMarksModifyLocalPhase::completed);
    assert(parsed.evidence.outcome ==
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown);
    assert(parsed.evidence.evidenceReference == "nmarks:state-extension:test");

    return 0;
}
