#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateLocalState.h"
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
    value.title = "Phase 64 CREATE local state";
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

BackendAgentLocalProviderSelection selection()
{
    BackendAgentLocalProviderSelection value;
    value.backendId = "default";
    value.authorityDomain = kBackendAgentNativeTimerCreateAuthorityDomain;
    value.providerId = kBackendAgentNativeTimerCreateProviderId;
    value.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    value.ownershipGeneration = 4;
    value.providerInstanceEpoch = "pie_create_local_1";
    value.providerGeneration = 7;
    value.capabilityRevision = 11;
    value.requiredCapability = kBackendAgentNativeTimerCreateCapability;
    return value;
}

BackendAgentCommandAssignment assignment()
{
    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = "2";
    payload.timerAssignmentId = "ta_create_local_1";
    payload.expectedAssignmentRevision = "4";
    payload.expectedIntentRevision = "9";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "ntb_create_local_1";
    payload.controlPlaneClaimedAt = 100;
    payload.specification = specification();
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification);
    payload.localProviderSelection = selection();

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_create_local_1";
    value.correlationId = value.requestId;
    value.operationId = "op_create_local_1";
    value.jobId = "job_create_local_1";
    value.attemptId = "attempt_create_local_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_create_local_1";
    value.backendId = "default";
    value.agentId = "agt_create_local_1";
    value.agentInstanceId = "inst_create_local_1";
    value.backendGeneration = 7;
    value.commandType = kBackendAgentNativeTimerCreateCommandType;
    value.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    value.payload = backendAgentNativeTimerCreatePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 101;
    value.deadline = 500;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentNativeTimerCreateEvidence evidenceFor(
    const BackendAgentNativeTimerCreateCommand& command,
    BackendAgentNativeTimerCreateOutcomeCategory outcome,
    std::int64_t dispatchStartedAt,
    const std::string& reference)
{
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
    evidence.outcome = outcome;
    evidence.dispatchStartedAt = dispatchStartedAt;
    evidence.completedAt = 120;
    evidence.evidenceReference = reference;
    return evidence;
}
}

int main()
{
    const auto assigned = assignment();
    BackendAgentNativeTimerCreateCommand command;
    std::string reason;
    assert(backendAgentNativeTimerCreateCommandFromAssignment(
        assigned, command, reason));
    assert(command.timerAssignmentId == "ta_create_local_1");
    assert(command.nativeTimerBindingId == "ntb_create_local_1");
    assert(command.localProviderSelection.providerInstanceEpoch ==
        "pie_create_local_1");

    BackendAgentNativeTimerCreateLocalState starting;
    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 110, starting, reason));
    assert(starting.phase == BackendAgentNativeTimerCreateLocalPhase::starting);
    const std::string encodedStarting =
        backendAgentNativeTimerCreateSerializeLocalState(starting, reason);
    assert(!encodedStarting.empty());

    BackendAgentNativeTimerCreateLocalState parsedStarting;
    assert(backendAgentNativeTimerCreateParseLocalState(
        encodedStarting, parsedStarting, reason));
    assert(backendAgentNativeTimerCreateSerializeLocalState(
        parsedStarting, reason) == encodedStarting);

    BackendAgentNativeTimerCreateEvidence recovered;
    const auto recovery = backendAgentNativeTimerCreateRecoverLocalState(
        parsedStarting, recovered, reason);
    assert(recovery == BackendAgentNativeTimerCreateRecoveryDecision::reconcileOnly);
    assert(reason ==
        "native_timer_create_dispatch_may_have_occurred_reconcile_only");
    assert(recovered.completedAt == 0);

    // Once local starting is durable, crash/restart is never evidence that the
    // native CREATE did not happen and must never authorize a second dispatch.
    auto accepted = evidenceFor(
        command,
        BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified,
        111,
        "suitebridge_create_accepted_unverified");
    assert(backendAgentNativeTimerCreateCompleteLocalState(
        starting, accepted, reason));
    assert(starting.phase == BackendAgentNativeTimerCreateLocalPhase::completed);
    const std::string encodedCompleted =
        backendAgentNativeTimerCreateSerializeLocalState(starting, reason);
    assert(!encodedCompleted.empty());

    BackendAgentNativeTimerCreateLocalState parsedCompleted;
    assert(backendAgentNativeTimerCreateParseLocalState(
        encodedCompleted, parsedCompleted, reason));
    const auto completedRecovery = backendAgentNativeTimerCreateRecoverLocalState(
        parsedCompleted, recovered, reason);
    assert(completedRecovery ==
        BackendAgentNativeTimerCreateRecoveryDecision::returnPersistedEvidence);
    assert(recovered.outcome ==
        BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified);
    assert(recovered.dispatchStartedAt == 111);
    assert(recovered.nativeTimerBindingId == "ntb_create_local_1");

    BackendAgentNativeTimerCreateLocalState rejectedStarting;
    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 110, rejectedStarting, reason));
    auto rejected = evidenceFor(
        command,
        BackendAgentNativeTimerCreateOutcomeCategory::rejectedWithoutEffect,
        0,
        "suitebridge_create_rejected_without_effect");
    assert(backendAgentNativeTimerCreateCompleteLocalState(
        rejectedStarting, rejected, reason));
    assert(backendAgentNativeTimerCreateLocalStateValid(rejectedStarting, reason));

    BackendAgentNativeTimerCreateLocalState unknownStarting;
    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 110, unknownStarting, reason));
    auto unknown = evidenceFor(
        command,
        BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown,
        112,
        "suitebridge_create_outcome_unknown");
    assert(backendAgentNativeTimerCreateCompleteLocalState(
        unknownStarting, unknown, reason));
    assert(backendAgentNativeTimerCreateLocalStateValid(unknownStarting, reason));

    BackendAgentNativeTimerCreateLocalState mismatchStarting;
    assert(backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 110, mismatchStarting, reason));
    auto wrongBinding = accepted;
    wrongBinding.nativeTimerBindingId = "ntb_create_local_other";
    assert(!backendAgentNativeTimerCreateCompleteLocalState(
        mismatchStarting, wrongBinding, reason));

    auto wrongProvider = accepted;
    wrongProvider.providerInstanceEpoch = "pie_create_local_other";
    assert(!backendAgentNativeTimerCreateCompleteLocalState(
        mismatchStarting, wrongProvider, reason));

    BackendAgentNativeTimerCreateLocalState invalidStarting;
    assert(!backendAgentNativeTimerCreatePrepareLocalStarting(
        command, 99, invalidStarting, reason));

    assert(!backendAgentNativeTimerCreateParseLocalState(
        encodedStarting + "x", parsedStarting, reason));
    std::string tampered = encodedStarting;
    const auto pos = tampered.find("pie_create_local_1");
    assert(pos != std::string::npos);
    tampered.replace(pos, std::string("pie_create_local_1").size(),
                     "pie_create_local_2");
    assert(!backendAgentNativeTimerCreateParseLocalState(
        tampered, parsedStarting, reason));

    return 0;
}
