#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreate.h"
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
    value.title = "Phase 64 CREATE";
    value.directory = "Tests";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "915";
    value.endTime = "1045";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

BackendAgentLocalProviderSelection selection()
{
    BackendAgentLocalProviderSelection value;
    value.backendId = "backend_1";
    value.authorityDomain = kBackendAgentNativeTimerCreateAuthorityDomain;
    value.providerId = kBackendAgentNativeTimerCreateProviderId;
    value.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    value.ownershipGeneration = 4;
    value.providerInstanceEpoch = "provider_epoch_9";
    value.providerGeneration = 7;
    value.capabilityRevision = 11;
    value.requiredCapability = kBackendAgentNativeTimerCreateCapability;
    return value;
}

BackendAgentNativeTimerCreatePayload payload()
{
    BackendAgentNativeTimerCreatePayload value;
    value.operationRevision = "1";
    value.timerAssignmentId = "assignment_1";
    value.expectedAssignmentRevision = "3";
    value.expectedIntentRevision = "8";
    value.assignmentEpoch = 2;
    value.nativeTimerBindingId = "binding_1";
    value.controlPlaneClaimedAt = 100;
    value.specification = specification();
    value.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(value.specification);
    value.localProviderSelection = selection();
    return value;
}
}

int main()
{
    const auto expectedSpecification = specification();
    assert(backendAgentNativeTimerCreateSpecificationValid(expectedSpecification));
    const std::string fingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(expectedSpecification);
    assert(fingerprint.find("native-timer-specification/1|") == 0);

    const auto expectedPayload = payload();
    assert(backendAgentNativeTimerCreatePayloadValid(expectedPayload));
    const std::string encoded = backendAgentNativeTimerCreatePayload(expectedPayload);
    assert(!encoded.empty());

    BackendAgentNativeTimerCreatePayload parsed;
    std::string reason;
    assert(backendAgentNativeTimerCreateParsePayload(encoded, parsed, reason));
    assert(parsed.timerAssignmentId == expectedPayload.timerAssignmentId);
    assert(parsed.nativeTimerBindingId == expectedPayload.nativeTimerBindingId);
    assert(parsed.expectedSpecificationFingerprint == fingerprint);
    assert(parsed.specification.startTime == "0915");
    assert(backendAgentNativeTimerCreatePayload(parsed) == encoded);

    BackendAgentNativeTimerCreatePayload badProvider = expectedPayload;
    badProvider.localProviderSelection.requiredCapability = "vdr.timer.delete";
    assert(backendAgentNativeTimerCreatePayload(badProvider).empty());
    assert(!backendAgentNativeTimerCreateParsePayload(encoded + "x", parsed, reason));

    BackendAgentCommandAssignment assignment;
    assignment.present = true;
    assignment.requestId = "req_1";
    assignment.correlationId = "req_1";
    assignment.operationId = "op_1";
    assignment.jobId = "job_1";
    assignment.attemptId = "att_1";
    assignment.claimEpoch = 1;
    assignment.commandId = "cmd_1";
    assignment.backendId = "backend_1";
    assignment.agentId = "agent_1";
    assignment.agentInstanceId = "instance_1";
    assignment.backendGeneration = 5;
    assignment.commandType = kBackendAgentNativeTimerCreateCommandType;
    assignment.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    assignment.payload = encoded;
    assignment.verificationPolicy = "readback_required";
    assignment.assignedAt = 101;
    assignment.deadline = 300;
    assignment.requestFingerprint = backendAgentCommandFingerprint(assignment);
    assert(backendAgentCommandValidAssignment(assignment));

    BackendAgentNativeTimerCreateCommand command;
    command.commandId = assignment.commandId;
    command.requestFingerprint = assignment.requestFingerprint;
    command.operationId = assignment.operationId;
    command.operationRevision = expectedPayload.operationRevision;
    command.timerAssignmentId = expectedPayload.timerAssignmentId;
    command.expectedAssignmentRevision = expectedPayload.expectedAssignmentRevision;
    command.expectedIntentRevision = expectedPayload.expectedIntentRevision;
    command.assignmentEpoch = expectedPayload.assignmentEpoch;
    command.nativeTimerBindingId = expectedPayload.nativeTimerBindingId;
    command.expectedSpecificationFingerprint = fingerprint;
    command.jobId = assignment.jobId;
    command.attemptId = assignment.attemptId;
    command.claimEpoch = assignment.claimEpoch;
    command.backendId = assignment.backendId;
    command.agentId = assignment.agentId;
    command.agentInstanceId = assignment.agentInstanceId;
    command.backendGeneration = assignment.backendGeneration;
    command.controlPlaneClaimedAt = expectedPayload.controlPlaneClaimedAt;
    command.specification = expectedPayload.specification;
    command.localProviderSelection = expectedPayload.localProviderSelection;
    assert(backendAgentNativeTimerCreateValidCommand(command, reason));

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
    evidence.localStartingPersistedAt = 110;
    evidence.outcome = BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified;
    evidence.dispatchStartedAt = 111;
    evidence.completedAt = 112;
    evidence.evidenceReference = "suitebridge-create-accepted";
    assert(backendAgentNativeTimerCreateEvidenceMatches(evidence, command, reason));
    evidence.providerInstanceEpoch = "stale_epoch";
    assert(!backendAgentNativeTimerCreateEvidenceMatches(evidence, command, reason));

    return 0;
}
