#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerDeletePayload.h"

#include <algorithm>
#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{

BackendAgentCommandAssignment assignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerDeleteAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerDeleteProviderId;
    selection.providerKind = kBackendAgentNativeTimerDeleteProviderKind;
    selection.ownershipGeneration = 31;
    selection.providerInstanceEpoch = "suitebridge-epoch-8";
    selection.providerGeneration = 12;
    selection.capabilityRevision = 5;
    selection.requiredCapability = kBackendAgentNativeTimerDeleteCapability;

    BackendAgentNativeTimerDeletePayload payload;
    payload.operationRevision = "op-rev-14";
    payload.nativeTimerBindingId = "binding-23";
    payload.expectedBindingRevision = "binding-rev-9";
    payload.expectedNativeTimerFingerprint = "sha256:native-timer-observed-state-extension";
    payload.timerAssignmentId = "timer-assignment-19";
    payload.backendNativeTimerId = "native-timer-72";
    payload.controlPlaneClaimedAt = 200;
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-28";
    value.correlationId = "correlation-28";
    value.operationId = "operation-28";
    value.jobId = "job-28";
    value.attemptId = "attempt-28";
    value.claimEpoch = 4;
    value.commandId = "command-28";
    value.backendId = "default";
    value.agentId = "agent-28";
    value.agentInstanceId = "instance-28";
    value.backendGeneration = 44;
    value.commandType = kBackendAgentNativeTimerDeleteCommandType;
    value.payloadVersion = kBackendAgentNativeTimerDeletePayloadVersion;
    value.payload = backendAgentNativeTimerDeletePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 210;
    value.deadline = 600;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

}

int main()
{
    const auto assigned = assignment();
    std::string reason;

    BackendAgentNativeTimerDeleteLocalState starting;
    assert(backendAgentNativeTimerDeletePrepareLocalStarting(
        assigned, 220, starting, reason));

    const std::string encoded =
        backendAgentNativeTimerDeleteCommandStateExtension(
            assigned, starting, reason);
    assert(!encoded.empty());
    assert(encoded.rfind("cse1.", 0) == 0);
    assert(encoded.find('\n') == std::string::npos);
    assert(encoded.find('=') == std::string::npos);
    assert(encoded.size() < 40U * 1024U);

    BackendAgentNativeTimerDeleteLocalState parsed;
    assert(backendAgentNativeTimerDeleteParseCommandStateExtension(
        encoded, assigned, parsed, reason));
    assert(parsed.phase == BackendAgentNativeTimerDeleteLocalPhase::starting);
    assert(parsed.localStartingPersistedAt == 220);
    assert(parsed.command.commandId == assigned.commandId);
    assert(parsed.command.requestFingerprint == assigned.requestFingerprint);
    assert(parsed.command.localProviderSelection.ownershipGeneration == 31);

    // The generic wrapper itself accepts bounded multi-line extension payloads
    // while emitting a single-line safe value for commands.state.
    BackendAgentCommandStateExtension generic;
    generic.extensionType = "test.extension.v1";
    generic.commandId = assigned.commandId;
    generic.requestFingerprint = assigned.requestFingerprint;
    generic.payload = "line-one\nline-two=value";
    const std::string genericEncoded =
        backendAgentCommandStateExtensionSerialize(generic, assigned, reason);
    assert(!genericEncoded.empty());
    assert(genericEncoded.find('\n') == std::string::npos);
    assert(genericEncoded.find('=') == std::string::npos);
    BackendAgentCommandStateExtension genericParsed;
    assert(backendAgentCommandStateExtensionParse(
        genericEncoded, assigned, genericParsed, reason));
    assert(genericParsed.extensionType == generic.extensionType);
    assert(genericParsed.payload == generic.payload);

    // Command identity is part of the wrapper fence; an assignment replay with
    // a different command identity cannot adopt an existing local extension.
    auto different = assigned;
    different.commandId = "command-other";
    different.requestFingerprint = backendAgentCommandFingerprint(different);
    assert(backendAgentCommandValidAssignment(different));
    assert(!backendAgentCommandStateExtensionParse(
        encoded, different, genericParsed, reason));

    // A syntactically valid extension for this assignment still cannot be
    // interpreted as Timer-delete local state unless its extension type is exact.
    generic.extensionType = "test.timer-delete-lookalike.v1";
    const std::string wrongType =
        backendAgentCommandStateExtensionSerialize(generic, assigned, reason);
    assert(!backendAgentNativeTimerDeleteParseCommandStateExtension(
        wrongType, assigned, parsed, reason));
    assert(reason == "native_timer_delete_state_extension_type_mismatch");

    // Tampering with one encoded payload nibble must fail closed either in the
    // generic codec or in the strict typed Timer-delete state parser.
    std::string tampered = encoded;
    const auto lastDot = tampered.rfind('.');
    assert(lastDot != std::string::npos && lastDot + 2 < tampered.size());
    tampered[lastDot + 2] = tampered[lastDot + 2] == '0' ? '1' : '0';
    assert(!backendAgentNativeTimerDeleteParseCommandStateExtension(
        tampered, assigned, parsed, reason));

    // The wrapper preserves completed evidence as well as starting state.
    auto completed = starting;
    BackendAgentNativeTimerDeleteEvidence evidence;
    evidence.commandId = completed.command.commandId;
    evidence.requestFingerprint = completed.command.requestFingerprint;
    evidence.operationId = completed.command.operationId;
    evidence.operationRevision = completed.command.operationRevision;
    evidence.jobId = completed.command.jobId;
    evidence.attemptId = completed.command.attemptId;
    evidence.claimEpoch = completed.command.claimEpoch;
    evidence.backendId = completed.command.backendId;
    evidence.agentId = completed.command.agentId;
    evidence.agentInstanceId = completed.command.agentInstanceId;
    evidence.backendGeneration = completed.command.backendGeneration;
    evidence.providerInstanceEpoch =
        completed.command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = completed.localStartingPersistedAt;
    evidence.outcome = BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified;
    evidence.dispatchStartedAt = 221;
    evidence.completedAt = 225;
    evidence.evidenceReference = "executor-evidence-28";
    assert(backendAgentNativeTimerDeleteCompleteLocalState(
        completed, evidence, reason));
    const std::string completedEncoded =
        backendAgentNativeTimerDeleteCommandStateExtension(
            assigned, completed, reason);
    assert(!completedEncoded.empty());
    assert(backendAgentNativeTimerDeleteParseCommandStateExtension(
        completedEncoded, assigned, parsed, reason));
    assert(parsed.phase == BackendAgentNativeTimerDeleteLocalPhase::completed);
    assert(parsed.evidence.evidenceReference == "executor-evidence-28");

    // Oversized extension payloads are rejected before encoding.
    generic.extensionType = "test.extension.v1";
    generic.payload.assign(16U * 1024U + 1U, 'x');
    assert(backendAgentCommandStateExtensionSerialize(
        generic, assigned, reason).empty());

    return 0;
}
