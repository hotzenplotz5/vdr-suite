#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateActivation.h"
#include "BackendAgentNativeTimerCreateReservation.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationCompletionService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateProductiveRuntime.h"
#include "NativeTimerCreateReadbackVerificationService.h"
#include "NativeTimerSpecification.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace vdrsuite::agent;
using namespace vdrsuite::daemon;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Phase 69 productive runtime";
    value.directory = "VDR-Suite";
    value.day = "2026-09-26";
    value.weekdays = "-------";
    value.startTime = "1930";
    value.endTime = "2015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerAssignment selectedAssignment(TimerAssignmentRepository& repository)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:phase69:productive";
    assignment.timerIntentId = "intent:phase69:productive";
    assignment.intentRevision = "1";
    assignment.backendId = "default";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:ard";
    assignment.channelBinding.backendChannelId = specification().channelId;
    assignment.channelBinding.mappingSource = "map";
    assignment.channelBinding.mappingRevision = "map:1";
    assignment.capabilityRevision = "cap:1";
    assignment.backendHealthRevision = "health:1";
    assignment.decisionPolicyVersion = "policy:1";
    assignment.decisionEvidence.reasons = {"selected"};
    assignment.createdAt = 90;
    assignment.updatedAt = 90;
    const auto created = repository.create(assignment);
    assert(created.ok());
    return created.assignment;
}

BackendAgentLocalProviderFacts providerFacts()
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = kBackendAgentNativeTimerCreateProviderId;
    facts.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    facts.providerInstanceEpoch = "pie_phase69_productive_1";
    facts.providerGeneration = 3;
    facts.capabilityRevision = 4;
    facts.available = true;
    facts.capabilities = {kBackendAgentNativeTimerCreateCapability};
    return facts;
}

void seedAgentAndProvider(
    Database& database,
    BackendAgentCommandRepository& commands)
{
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_phase69_productive','default','actor_phase69_productive',"
        "'dev_phase69_productive','cred_phase69_productive',1,"
        "'inst_phase69_productive',7,'vdr-suite-agent/1','test',"
        "2,1,100,100,1000,1,1);"));

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

    BackendAgentCommandPollRequest observation;
    observation.backendId = "default";
    observation.agentInstanceId = "inst_phase69_productive";
    observation.backendGeneration = 7;
    observation.supportedCommandTypes = {"probe.noop"};
    observation.localProviders = {providerFacts()};
    const auto observed =
        commands.poll(observation, "agt_phase69_productive", 102);
    assert(observed.accepted);
    assert(!observed.assignment.present);
}

NativeTimerObservedState observedState()
{
    const auto expected = specification();
    NativeTimerObservedState value;
    value.channelId = expected.channelId;
    value.title = expected.title;
    value.directory = expected.directory;
    value.day = expected.day;
    value.weekdays = expected.weekdays;
    value.startTime = expected.startTime;
    value.endTime = expected.endTime;
    value.flags = 1;
    value.priority = expected.priority;
    value.lifetime = expected.lifetime;
    value.enabled = expected.enabled;
    value.vps = expected.vps;
    return value;
}

NativeTimerCreateReadbackEvidence readbackEvidence(
    const TimerAssignment& assignment,
    const std::string& nativeTimerBindingId,
    std::int64_t observedAt)
{
    NativeTimerCreateReadbackCandidate candidate;
    candidate.timerAssignmentId = assignment.timerAssignmentId;
    candidate.nativeTimerBindingId = nativeTimerBindingId;
    candidate.observation.backendId = assignment.backendId;
    candidate.observation.backendGeneration = assignment.backendGeneration;
    candidate.observation.backendNativeTimerId = "native:phase69:productive:42";
    candidate.observation.observedAt = observedAt;
    candidate.observation.observedState = observedState();
    candidate.observation.observedFingerprint =
        nativeTimerObservedStateFingerprint(candidate.observation.observedState);

    NativeTimerCreateReadbackEvidence evidence;
    evidence.backendId = assignment.backendId;
    evidence.backendGeneration = assignment.backendGeneration;
    evidence.observedAt = observedAt;
    evidence.completeness = NativeTimerCreateReadbackCompleteness::complete;
    evidence.candidates = {candidate};
    return evidence;
}

BackendAgentCommandReceipt receiptFor(
    const BackendAgentCommandAssignment& assignment)
{
    BackendAgentCommandReceipt receipt;
    receipt.commandId = assignment.commandId;
    receipt.requestFingerprint = assignment.requestFingerprint;
    receipt.jobId = assignment.jobId;
    receipt.attemptId = assignment.attemptId;
    receipt.claimEpoch = assignment.claimEpoch;
    receipt.backendId = assignment.backendId;
    receipt.agentId = assignment.agentId;
    receipt.agentInstanceId = assignment.agentInstanceId;
    receipt.backendGeneration = assignment.backendGeneration;
    receipt.receiptCategory = "accepted";
    receipt.receivedAt = 121;
    receipt.reasonCode = "durably_recorded_before_native_create";
    assert(backendAgentCommandValidReceipt(receipt));
    return receipt;
}

BackendAgentCommandResult acceptedUnverifiedResult(
    const BackendAgentCommandAssignment& assignment)
{
    BackendAgentCommandResult result;
    result.commandId = assignment.commandId;
    result.requestFingerprint = assignment.requestFingerprint;
    result.jobId = assignment.jobId;
    result.attemptId = assignment.attemptId;
    result.claimEpoch = assignment.claimEpoch;
    result.backendId = assignment.backendId;
    result.agentId = assignment.agentId;
    result.agentInstanceId = assignment.agentInstanceId;
    result.backendGeneration = assignment.backendGeneration;
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
        assignment, command, reason));

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
    evidence.evidenceReference = "suitebridge:ntcreate:phase69-productive";
    result.resultEvidence = backendAgentNativeTimerCreateResultEvidence(
        evidence, assignment, reason);
    assert(!result.resultEvidence.empty());
    assert(backendAgentCommandValidResult(result));
    return result;
}
} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));
    assert(database.execute(
        "CREATE TABLE timer_intents ("
        "timer_intent_id TEXT PRIMARY KEY NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision > 0)"
        ");"));
    assert(database.execute(
        "INSERT INTO timer_intents "
        "(timer_intent_id,intent_revision) "
        "VALUES ('intent:phase69:productive',1);"));

    MutationOperationRepository operations(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    BackendAgentCommandReservationRepository reservations(database);

    assert(operations.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(reservations.ensureSchema());

    const TimerAssignment selected = selectedAssignment(assignments);
    TimerAssignmentFulfillmentService fulfillment(assignments, bindings);
    const auto provisioning = fulfillment.beginProvisioning(
        selected.timerAssignmentId,
        selected.assignmentRevision,
        selected.intentRevision,
        selected.backendGeneration,
        95);
    assert(provisioning.status ==
        TimerAssignmentFulfillmentStatus::provisioningStarted);
    const TimerAssignment assignment = provisioning.assignment;

    NativeTimerCreateOperationPayload payload;
    payload.timerAssignmentId = assignment.timerAssignmentId;
    payload.expectedAssignmentRevision = assignment.assignmentRevision;
    payload.expectedIntentRevision = assignment.intentRevision;
    payload.assignmentEpoch = assignment.assignmentEpoch;
    payload.nativeTimerBindingId = "binding:phase69:productive";
    payload.backendId = assignment.backendId;
    payload.backendGeneration = assignment.backendGeneration;
    payload.expectedSpecification = specification();

    MutationOperation operation;
    operation.operationId = "operation:phase69:productive";
    operation.idempotencyKey = "idempotency:phase69:productive";
    operation.actorId = "actor:phase69";
    operation.backendId = assignment.backendId;
    operation.backendGeneration = assignment.backendGeneration;
    operation.resourceType = "TimerAssignment";
    operation.resourceId = assignment.timerAssignmentId;
    operation.expectedRevision = assignment.assignmentRevision;
    operation.expectedResourceFingerprint =
        nativeTimerSpecificationFingerprint(specification());
    operation.actionFamily = "timer.create";
    operation.requestFingerprint = "request:phase69:productive";
    operation.requestedAt = 100;
    operation.deadline = 700;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;
    operation.state = MutationOperationState::accepted;
    operation.updatedAt = 100;

    MutationOperationPayload durable;
    durable.operationId = operation.operationId;
    durable.payloadType = "native.timer.create";
    durable.payloadVersion = 1;
    durable.payload = serializeNativeTimerCreateOperationPayload(payload);
    durable.payloadFingerprint =
        nativeTimerCreateOperationPayloadFingerprint(payload);
    assert(operations.reserveWithPayload(operation, durable).ok());

    seedAgentAndProvider(database, commands);

    BackendAgentNativeTimerCreateReservationService reservationService(
        commands, reservations, agents);
    NativeTimerCreateDispatchService dispatch(operations);
    BackendAgentNativeTimerCreateActivationService activation(
        operations, reservations, commands);
    NativeTimerCreateReadbackVerificationService verification(bindings);
    NativeTimerCreateOperationCompletionService completion(
        operations, assignments, bindings);

    const NativeTimerCreateReadbackAcquirer noReadback =
        [](const NativeTimerCreateReadbackExpectation&)
            -> std::optional<NativeTimerCreateReadbackEvidence>
        {
            return std::nullopt;
        };

    const auto activated = advanceNativeTimerCreateRuntimeOnce(
        operations,
        commands,
        reservationService,
        dispatch,
        activation,
        verification,
        fulfillment,
        completion,
        noReadback,
        120);
    assert(activated.discovered == 1);
    assert(activated.dispatchClaims == 1);
    assert(activated.activations == 1);

    const auto dispatching = operations.findById(operation.operationId);
    assert(dispatching.ok());
    assert(dispatching.operation.state == MutationOperationState::dispatching);
    assert(dispatching.operation.operationRevision == "2");

    const auto command = commands.findAssignmentForOperation(
        "default",
        operation.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    assert(command.has_value());
    const std::string exactCommandId = command->commandId;

    const auto waiting = advanceNativeTimerCreateRuntimeOnce(
        operations,
        commands,
        reservationService,
        dispatch,
        activation,
        verification,
        fulfillment,
        completion,
        noReadback,
        122);
    assert(waiting.discovered == 1);
    assert(waiting.dispatchClaims == 0);
    assert(waiting.activations == 0);
    const auto sameCommand = commands.findAssignmentForOperation(
        "default",
        operation.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    assert(sameCommand.has_value());
    assert(sameCommand->commandId == exactCommandId);

    assert(commands.acceptReceipt(receiptFor(*command)).accepted);
    assert(commands.acceptResult(
        acceptedUnverifiedResult(*command)).accepted);

    const auto authoritativeReadback =
        readbackEvidence(assignment, payload.nativeTimerBindingId, 135);
    const NativeTimerCreateReadbackAcquirer readback =
        [authoritativeReadback](
            const NativeTimerCreateReadbackExpectation& expectation)
            -> std::optional<NativeTimerCreateReadbackEvidence>
        {
            assert(expectation.operationId ==
                "operation:phase69:productive");
            assert(expectation.readbackNotBefore == 124);
            return authoritativeReadback;
        };

    const auto completed = advanceNativeTimerCreateRuntimeOnce(
        operations,
        commands,
        reservationService,
        dispatch,
        activation,
        verification,
        fulfillment,
        completion,
        readback,
        140);
    assert(completed.discovered == 1);
    assert(completed.dispatchClaims == 0);
    assert(completed.activations == 0);
    assert(completed.outcomesApplied == 1);
    assert(completed.reconciliationsCompleted == 1);

    const auto succeeded = operations.findById(operation.operationId);
    assert(succeeded.ok());
    assert(succeeded.operation.state == MutationOperationState::succeeded);
    assert(succeeded.operation.operationRevision == "4");

    const auto bound = assignments.findById(assignment.timerAssignmentId);
    assert(bound.ok());
    assert(bound.assignment.state == TimerAssignmentState::bound);
    assert(bound.assignment.nativeTimerBindingId ==
        payload.nativeTimerBindingId);

    const auto binding = bindings.findById(payload.nativeTimerBindingId);
    assert(binding.ok());
    assert(binding.binding.ownership == NativeTimerBindingOwnership::managed);
    assert(binding.binding.lastVerifiedOperationId == operation.operationId);
    assert(binding.binding.backendNativeTimerId ==
        "native:phase69:productive:42");

    const auto terminalReplay = advanceNativeTimerCreateRuntimeOnce(
        operations,
        commands,
        reservationService,
        dispatch,
        activation,
        verification,
        fulfillment,
        completion,
        readback,
        145);
    assert(terminalReplay.discovered == 0);
    const auto stillSameCommand = commands.findAssignmentForOperation(
        "default",
        operation.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    assert(stillSameCommand.has_value());
    assert(stillSameCommand->commandId == exactCommandId);

    std::cout
        << "test_native_timer_create_productive_runtime passed\n";
    return 0;
}
