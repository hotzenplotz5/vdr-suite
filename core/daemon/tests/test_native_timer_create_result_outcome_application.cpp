#include "BackendAgentCommandDelivery.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateResultOutcomeApplication.h"
#include "NativeTimerSpecification.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace vdrsuite::agent;
using namespace vdrsuite::daemon;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

BackendAgentLocalProviderFacts providerFacts()
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = kBackendAgentNativeTimerCreateProviderId;
    facts.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    facts.providerInstanceEpoch = "pie_outcome_application_1";
    facts.providerGeneration = 3;
    facts.capabilityRevision = 4;
    facts.available = true;
    facts.capabilities = {kBackendAgentNativeTimerCreateCapability};
    return facts;
}

NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 69 outcome application";
    value.directory = "Tests";
    value.day = "2026-09-25";
    value.weekdays = "-------";
    value.startTime = "1230";
    value.endTime = "1315";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

BackendAgentNativeTimerCreateSpecification agentSpecification()
{
    const auto source = specification();
    BackendAgentNativeTimerCreateSpecification value;
    value.channelId = source.channelId;
    value.title = source.title;
    value.directory = source.directory;
    value.day = source.day;
    value.weekdays = source.weekdays;
    value.startTime = source.startTime;
    value.endTime = source.endTime;
    value.priority = source.priority;
    value.lifetime = source.lifetime;
    value.enabled = source.enabled;
    value.vps = source.vps;
    return value;
}

MutationOperation acceptedOperation()
{
    MutationOperation value;
    value.operationId = "op_outcome_application_1";
    value.idempotencyKey = "idem_outcome_application_1";
    value.actorId = "actor_outcome_application";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.resourceType = "TimerAssignment";
    value.resourceId = "ta_outcome_application_1";
    value.expectedRevision = "4";
    value.expectedResourceFingerprint =
        nativeTimerSpecificationFingerprint(specification());
    value.actionFamily = "timer.create";
    value.requestFingerprint = "request_outcome_application_1";
    value.requestedAt = 100;
    value.deadline = 500;
    value.verificationPolicy = MutationOperationVerificationPolicy::readbackRequired;
    value.state = MutationOperationState::accepted;
    value.updatedAt = 100;
    return value;
}

NativeTimerCreateOperationPayload operationPayload()
{
    NativeTimerCreateOperationPayload value;
    value.timerAssignmentId = "ta_outcome_application_1";
    value.expectedAssignmentRevision = "4";
    value.expectedIntentRevision = "9";
    value.assignmentEpoch = 3;
    value.nativeTimerBindingId = "ntb_outcome_application_1";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.expectedSpecification = specification();
    return value;
}

BackendAgentCommandAssignment commandAssignment(
    const BackendAgentLocalProviderSelection& selection,
    const std::string& reservedOperationRevision)
{
    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = reservedOperationRevision;
    payload.timerAssignmentId = "ta_outcome_application_1";
    payload.expectedAssignmentRevision = "4";
    payload.expectedIntentRevision = "9";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "ntb_outcome_application_1";
    payload.controlPlaneClaimedAt = 109;
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(agentSpecification());
    payload.specification = agentSpecification();
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_outcome_application_1";
    value.correlationId = value.requestId;
    value.operationId = "op_outcome_application_1";
    value.jobId = "job_outcome_application_1";
    value.attemptId = "attempt_outcome_application_1";
    value.claimEpoch = 1;
    value.commandId = "cmd_outcome_application_1";
    value.backendId = "default";
    value.agentId = "agt_outcome_application";
    value.agentInstanceId = "inst_outcome_application";
    value.backendGeneration = 7;
    value.commandType = kBackendAgentNativeTimerCreateCommandType;
    value.payloadVersion = kBackendAgentNativeTimerCreatePayloadVersion;
    value.payload = backendAgentNativeTimerCreatePayload(payload);
    value.verificationPolicy = "readback_required";
    value.assignedAt = 110;
    value.deadline = 500;
    value.requestFingerprint = backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}

BackendAgentCommandReceipt receiptFor(const BackendAgentCommandAssignment& value)
{
    BackendAgentCommandReceipt receipt;
    receipt.commandId = value.commandId;
    receipt.requestFingerprint = value.requestFingerprint;
    receipt.jobId = value.jobId;
    receipt.attemptId = value.attemptId;
    receipt.claimEpoch = value.claimEpoch;
    receipt.backendId = value.backendId;
    receipt.agentId = value.agentId;
    receipt.agentInstanceId = value.agentInstanceId;
    receipt.backendGeneration = value.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = 121;
    receipt.reasonCode = "durably_recorded_before_native_create";
    assert(backendAgentCommandValidReceipt(receipt));
    return receipt;
}

BackendAgentCommandResult resultFor(const BackendAgentCommandAssignment& value)
{
    BackendAgentCommandResult result;
    result.commandId = value.commandId;
    result.requestFingerprint = value.requestFingerprint;
    result.jobId = value.jobId;
    result.attemptId = value.attemptId;
    result.claimEpoch = value.claimEpoch;
    result.backendId = value.backendId;
    result.agentId = value.agentId;
    result.agentInstanceId = value.agentInstanceId;
    result.backendGeneration = value.backendGeneration;
    result.dispatchState = "accepted_by_executor";
    result.verificationState = "outcome_unknown";
    result.resultCategory = "outcome_unknown";
    result.errorCategory = "none";
    result.retryClassification = "reconcile_only";
    result.boundedDiagnostics =
        "CREATE dispatched; authoritative readback required";
    result.completedAt = 130;

    BackendAgentNativeTimerCreateCommand command;
    std::string reason;
    assert(backendAgentNativeTimerCreateCommandFromAssignment(
        value, command, reason));

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
    evidence.providerInstanceEpoch =
        command.localProviderSelection.providerInstanceEpoch;
    evidence.localStartingPersistedAt = 123;
    evidence.outcome =
        BackendAgentNativeTimerCreateOutcomeCategory::acceptedUnverified;
    evidence.dispatchStartedAt = 124;
    evidence.completedAt = result.completedAt;
    evidence.evidenceReference = "suitebridge:ntcreate:outcome-application";
    result.resultEvidence = backendAgentNativeTimerCreateResultEvidence(
        evidence, value, reason);
    assert(!result.resultEvidence.empty());
    assert(backendAgentCommandValidResult(result));
    return result;
}
} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    MutationOperationRepository operations(database);
    BackendAgentCommandRepository commands(database);
    assert(operations.ensureSchema());
    assert(commands.ensureSchema());

    const auto operation = acceptedOperation();
    const auto payload = operationPayload();
    MutationOperationPayload durablePayload;
    durablePayload.operationId = operation.operationId;
    durablePayload.payloadType = "native.timer.create";
    durablePayload.payloadVersion = 1;
    durablePayload.payload = serializeNativeTimerCreateOperationPayload(payload);
    durablePayload.payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(payload);
    const auto reservedOperation =
        operations.reserveWithPayload(operation, durablePayload);
    assert(reservedOperation.ok());
    assert(reservedOperation.operation.operationRevision == "1");

    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default",
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateProviderId,
        kBackendAgentNativeTimerCreateProviderKind,
        {kBackendAgentNativeTimerCreateCapability},
        101,
        ownership,
        reason));

    BackendAgentCommandPollRequest providerObservation;
    providerObservation.backendId = "default";
    providerObservation.agentInstanceId = "inst_outcome_application";
    providerObservation.backendGeneration = 7;
    providerObservation.supportedCommandTypes = {"probe.noop"};
    providerObservation.localProviders = {providerFacts()};
    const auto observed = commands.poll(
        providerObservation, "agt_outcome_application", 102);
    assert(observed.accepted);
    assert(!observed.assignment.present);

    const auto selection = commands.selectLocalProvider(
        "default",
        "agt_outcome_application",
        "inst_outcome_application",
        7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability,
        reason);
    assert(selection.has_value());

    const auto assignment = commandAssignment(
        *selection, reservedOperation.operation.operationRevision);
    assert(commands.insertAssignment(assignment, &*selection));

    NativeTimerCreateDispatchService dispatch(operations);
    NativeTimerCreateDispatchClaimRequest claim;
    claim.operationId = operation.operationId;
    claim.expectedOperationRevision =
        reservedOperation.operation.operationRevision;
    claim.timerAssignmentId = payload.timerAssignmentId;
    claim.nativeTimerBindingId = payload.nativeTimerBindingId;
    claim.backendId = payload.backendId;
    claim.backendGeneration = payload.backendGeneration;
    claim.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(payload.expectedSpecification);
    claim.reservation.commandId = assignment.commandId;
    claim.reservation.requestFingerprint = assignment.requestFingerprint;

    const auto claimed = dispatch.claimAfterReservation(claim, 120);
    assert(claimed.status == NativeTimerCreateDispatchClaimStatus::claimed);
    assert(claimed.operation.operationRevision == "2");

    assert(commands.acceptReceipt(receiptFor(assignment)).accepted);
    const auto executorResult = resultFor(assignment);
    assert(commands.acceptResult(executorResult).accepted);

    const auto persisted = commands.resultForCommand(assignment.commandId);
    assert(persisted.has_value());
    BackendAgentNativeTimerCreateEvidence parsedEvidence;
    assert(backendAgentNativeTimerCreateParseResultEvidence(
        persisted->resultEvidence, assignment, parsedEvidence, reason));
    assert(parsedEvidence.operationRevision == "1");

    const auto beforeApplication = operations.findById(operation.operationId);
    assert(beforeApplication.ok());
    assert(beforeApplication.operation.state == MutationOperationState::dispatching);
    assert(beforeApplication.operation.operationRevision == "2");

    const auto applied = applyDurableNativeTimerCreateResult(
        commands, operations, dispatch, assignment.commandId);
    assert(applied.status ==
        NativeTimerCreateResultOutcomeApplicationStatus::applied);
    assert(applied.dispatch.expectationPresent);
    assert(applied.dispatch.expectation.readbackNotBefore == 124);
    assert(applied.dispatch.expectation.operationId == operation.operationId);

    const auto afterApplication = operations.findById(operation.operationId);
    assert(afterApplication.ok());
    assert(afterApplication.operation.state ==
        MutationOperationState::executedUnverified);
    assert(afterApplication.operation.operationRevision == "3");
    assert(afterApplication.operation.resultReference ==
        "suitebridge:ntcreate:outcome-application");

    const auto replay = applyDurableNativeTimerCreateResult(
        commands, operations, dispatch, assignment.commandId);
    assert(replay.status ==
        NativeTimerCreateResultOutcomeApplicationStatus::alreadyApplied);
    const auto afterReplay = operations.findById(operation.operationId);
    assert(afterReplay.ok());
    assert(afterReplay.operation.operationRevision == "3");

    return 0;
}
