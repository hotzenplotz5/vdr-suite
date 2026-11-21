#include "BackendAgentRecordingMarksModifyLocalState.h"
#include "BackendAgentRecordingMarksModifyPayload.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{

BackendAgentRecordingMarksModifyPayload payload()
{
    BackendAgentRecordingMarksModifyPayload value;
    value.kind = BackendAgentRecordingMarksModifyKind::replace;
    value.operationRevision = "4";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.replacementFrames = {10, 20, 30, 40};
    value.backendId = "default";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 100;
    auto& selection = value.localProviderSelection;
    selection.backendId = value.backendId;
    selection.authorityDomain = kBackendAgentRecordingMarksModifyAuthorityDomain;
    selection.providerId = kBackendAgentRecordingMarksModifyProviderId;
    selection.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
    selection.ownershipGeneration = 1;
    selection.providerInstanceEpoch = "pie_marks_1";
    selection.providerGeneration = 2;
    selection.capabilityRevision = 3;
    selection.requiredCapability = kBackendAgentRecordingMarksModifyCapability;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_marks_1";
    value.correlationId = "req_marks_1";
    value.operationId = "op_marks_1";
    value.commandType = kBackendAgentRecordingMarksModifyCommandType;
    value.payloadVersion = kBackendAgentRecordingMarksModifyPayloadVersion;
    value.payload = backendAgentRecordingMarksModifyPayload(payload());
    value.verificationPolicy = "readback_required";
    value.jobId = "job_marks_1";
    value.attemptId = "attempt_marks_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_marks_1";
    value.backendId = "default";
    value.agentId = "agent_marks_1";
    value.agentInstanceId = "instance_marks_1";
    value.backendGeneration = 7;
    value.assignedAt = 100;
    value.deadline = 1000;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    return value;
}

BackendAgentRecordingMarksModifyEvidence acceptedEvidence(
    const BackendAgentRecordingMarksModifyCommand& command,
    std::int64_t startingAt)
{
    BackendAgentRecordingMarksModifyEvidence evidence;
    evidence.commandId = command.commandId;
    evidence.requestFingerprint = command.requestFingerprint;
    evidence.operationId = command.operationId;
    evidence.operationRevision = command.operationRevision;
    evidence.jobId = command.jobId;
    evidence.attemptId = command.attemptId;
    evidence.claimEpoch = command.claimEpoch;
    evidence.backendId = command.backendId;
    evidence.agentId = command.agentId;
    evidence.agentInstanceId = command.agentInstanceId;
    evidence.backendGeneration = command.backendGeneration;
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = startingAt;
    evidence.outcome =
        BackendAgentRecordingMarksModifyOutcomeCategory::acceptedUnverified;
    evidence.dispatchStartedAt = startingAt + 1;
    evidence.completedAt = startingAt + 2;
    evidence.evidenceReference = "nmarks:test:accepted";
    return evidence;
}

}

int main()
{
    auto commandAssignment = assignment();
    assert(backendAgentCommandValidAssignment(commandAssignment));

    BackendAgentRecordingMarksModifyCommand command;
    std::string reasonCode;
    assert(backendAgentRecordingMarksModifyCommandFromAssignment(
        commandAssignment, command, reasonCode));
    assert(command.kind == BackendAgentRecordingMarksModifyKind::replace);
    assert(command.replacementFrames.size() == 4);
    assert(command.expectedMarksRevision ==
        "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

    BackendAgentRecordingMarksModifyLocalState starting;
    assert(backendAgentRecordingMarksModifyPrepareLocalStarting(
        commandAssignment, 101, starting, reasonCode));
    assert(starting.phase == BackendAgentRecordingMarksModifyLocalPhase::starting);
    assert(starting.localStartingPersistedAt == 101);

    const auto encoded = backendAgentRecordingMarksModifySerializeLocalState(
        starting, reasonCode);
    assert(!encoded.empty());
    BackendAgentRecordingMarksModifyLocalState parsed;
    assert(backendAgentRecordingMarksModifyParseLocalState(
        encoded, parsed, reasonCode));
    assert(parsed.command.commandId == commandAssignment.commandId);
    assert(parsed.command.requestFingerprint ==
        commandAssignment.requestFingerprint);
    assert(parsed.command.replacementFrames ==
        std::vector<int>({10, 20, 30, 40}));

    const auto recovery = backendAgentRecordingMarksModifyRecoverLocalState(
        parsed,
        "default",
        "agent_marks_1",
        "instance_marks_1",
        7,
        102);
    assert(recovery.decision ==
        BackendAgentRecordingMarksModifyRecoveryDecision::reconcileOnly);
    assert(recovery.reasonCode ==
        "recording_marks_modify_starting_recovery_reconcile_only");
    assert(recovery.evidence.outcome ==
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown);

    const auto fencedRecovery = backendAgentRecordingMarksModifyRecoverLocalState(
        parsed,
        "default",
        "agent_marks_1",
        "replacement_instance",
        7,
        103);
    assert(fencedRecovery.decision ==
        BackendAgentRecordingMarksModifyRecoveryDecision::reconcileOnly);
    assert(fencedRecovery.reasonCode ==
        "recording_marks_modify_starting_context_fenced_reconcile_only");

    const auto evidence = acceptedEvidence(
        parsed.command, parsed.localStartingPersistedAt);
    assert(backendAgentRecordingMarksModifyCompleteLocalState(
        parsed, evidence, reasonCode));
    assert(parsed.phase == BackendAgentRecordingMarksModifyLocalPhase::completed);

    const auto completedEncoded =
        backendAgentRecordingMarksModifySerializeLocalState(parsed, reasonCode);
    BackendAgentRecordingMarksModifyLocalState completedParsed;
    assert(backendAgentRecordingMarksModifyParseLocalState(
        completedEncoded, completedParsed, reasonCode));
    assert(completedParsed.evidence.evidenceReference ==
        "nmarks:test:accepted");

    const auto completedRecovery =
        backendAgentRecordingMarksModifyRecoverLocalState(
            completedParsed,
            "default",
            "agent_marks_1",
            "instance_marks_1",
            7,
            110);
    assert(completedRecovery.decision ==
        BackendAgentRecordingMarksModifyRecoveryDecision::returnPersistedEvidence);
    assert(completedRecovery.evidence.evidenceReference ==
        "nmarks:test:accepted");

    auto corrupt = completedEncoded;
    const auto marker = corrupt.find("claim_epoch=1");
    assert(marker != std::string::npos);
    corrupt.replace(marker, std::string("claim_epoch=1").size(), "claim_epoch=0");
    BackendAgentRecordingMarksModifyLocalState rejected;
    assert(!backendAgentRecordingMarksModifyParseLocalState(
        corrupt, rejected, reasonCode));

    return 0;
}
