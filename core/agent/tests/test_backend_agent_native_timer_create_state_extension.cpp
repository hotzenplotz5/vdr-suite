#include "BackendAgentCommandStateExtension.h"
#include "BackendAgentNativeTimerCreatePayload.h"

#include <cassert>
#include <string>

using namespace vdrsuite::agent;

namespace
{
BackendAgentNativeTimerCreateSpecification specification()
{
    BackendAgentNativeTimerCreateSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 CREATE extension";
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "0930";
    value.endTime = "1030";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentLocalProviderSelection selection;
    selection.backendId = "default";
    selection.authorityDomain = kBackendAgentNativeTimerCreateAuthorityDomain;
    selection.providerId = kBackendAgentNativeTimerCreateProviderId;
    selection.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    selection.ownershipGeneration = 31;
    selection.providerInstanceEpoch = "suitebridge-create-epoch-8";
    selection.providerGeneration = 12;
    selection.capabilityRevision = 5;
    selection.requiredCapability = kBackendAgentNativeTimerCreateCapability;

    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = "14";
    payload.timerAssignmentId = "timer-assignment-create-19";
    payload.expectedAssignmentRevision = "9";
    payload.expectedIntentRevision = "7";
    payload.assignmentEpoch = 6;
    payload.nativeTimerBindingId = "binding-create-23";
    payload.controlPlaneClaimedAt = 200;
    payload.specification = specification();
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification);
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.protocolVersion = "vdr-suite-agent/1";
    value.requestId = "request-create-28";
    value.correlationId = "correlation-create-28";
    value.operationId = "operation-create-28";
    value.jobId = "job-create-28";
    value.attemptId = "attempt-create-28";
    value.claimEpoch = 4;
    value.commandId = "command-create-28";
    value.backendId = "default";
    value.agentId = "agent-create-28";
    value.agentInstanceId = "instance-create-28";
    value.backendGeneration = 44;
    value.commandType = kBackendAgentNativeTimerCreateCommandType;
    value.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    value.payload = backendAgentNativeTimerCreatePayload(payload);
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
    BackendAgentNativeTimerCreateCommand command;
    assert(backendAgentNativeTimerCreateCommandFromAssignment(
        assigned, command, reason));

    BackendAgentNativeTimerCreateLocalState starting;
    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 220, starting, reason));

    const std::string encoded =
        backendAgentNativeTimerCreateCommandStateExtension(
            assigned, starting, reason);
    assert(!encoded.empty());
    assert(encoded.rfind("cse1.", 0) == 0);
    assert(encoded.find('\n') == std::string::npos);
    assert(encoded.find('=') == std::string::npos);

    BackendAgentNativeTimerCreateLocalState parsed;
    assert(backendAgentNativeTimerCreateParseCommandStateExtension(
        encoded, assigned, parsed, reason));
    assert(parsed.phase == BackendAgentNativeTimerCreateLocalPhase::starting);
    assert(parsed.command.timerAssignmentId == "timer-assignment-create-19");
    assert(parsed.command.expectedAssignmentRevision == "9");
    assert(parsed.command.expectedIntentRevision == "7");
    assert(parsed.command.assignmentEpoch == 6);
    assert(parsed.command.nativeTimerBindingId == "binding-create-23");
    assert(parsed.command.expectedSpecificationFingerprint ==
        backendAgentNativeTimerCreateSpecificationFingerprint(specification()));
    assert(parsed.command.localProviderSelection.providerInstanceEpoch ==
        "suitebridge-create-epoch-8");

    BackendAgentCommandStateExtension generic;
    assert(backendAgentCommandStateExtensionParse(
        encoded, assigned, generic, reason));
    assert(generic.extensionType ==
        kBackendAgentNativeTimerCreateLocalStateExtensionType);
    assert(backendAgentCommandStateExtensionValidateSupported(
        generic, assigned, reason));

    // Cross-command or cross-payload adoption must fail even when the generic
    // envelope itself remains syntactically valid.
    auto different = assigned;
    different.commandId = "command-create-other";
    different.requestFingerprint = backendAgentCommandFingerprint(different);
    assert(backendAgentCommandValidAssignment(different));
    assert(!backendAgentNativeTimerCreateParseCommandStateExtension(
        encoded, different, parsed, reason));

    auto changedPayload = assigned;
    BackendAgentNativeTimerCreatePayload payload;
    assert(backendAgentNativeTimerCreateParsePayload(
        changedPayload.payload, payload, reason));
    payload.nativeTimerBindingId = "binding-create-other";
    changedPayload.payload = backendAgentNativeTimerCreatePayload(payload);
    changedPayload.requestFingerprint = backendAgentCommandFingerprint(changedPayload);
    assert(backendAgentCommandValidAssignment(changedPayload));
    assert(!backendAgentNativeTimerCreateParseCommandStateExtension(
        encoded, changedPayload, parsed, reason));

    // Recovery evidence is embedded only after it has been conservatively
    // classified outcome_unknown; parsing the extension can never authorize a
    // second native CREATE.
    const auto recovery = backendAgentNativeTimerCreateRecoverLocalState(
        starting,
        assigned.backendId,
        assigned.agentId,
        assigned.agentInstanceId,
        assigned.backendGeneration,
        230);
    assert(recovery.decision ==
        BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly);
    assert(recovery.evidence.outcome ==
        BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown);
    assert(recovery.evidence.dispatchStartedAt == 220);
    auto completed = starting;
    assert(backendAgentNativeTimerCreateCompleteLocalState(
        completed, recovery.evidence, reason));
    const std::string completedEncoded =
        backendAgentNativeTimerCreateCommandStateExtension(
            assigned, completed, reason);
    assert(!completedEncoded.empty());
    assert(backendAgentNativeTimerCreateParseCommandStateExtension(
        completedEncoded, assigned, parsed, reason));
    assert(parsed.phase == BackendAgentNativeTimerCreateLocalPhase::completed);
    assert(parsed.evidence.outcome ==
        BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown);
    assert(parsed.evidence.evidenceReference ==
        "local-recovery:command-create-28");

    std::string tampered = encoded;
    const auto lastDot = tampered.rfind('.');
    assert(lastDot != std::string::npos && lastDot + 2 < tampered.size());
    tampered[lastDot + 2] = tampered[lastDot + 2] == '0' ? '1' : '0';
    assert(!backendAgentNativeTimerCreateParseCommandStateExtension(
        tampered, assigned, parsed, reason));

    return 0;
}
