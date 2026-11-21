#include "BackendAgentCommandDelivery.h"
#include "BackendAgentCommandReservation.h"
#include "BackendAgentNativeTimerCreate.h"
#include "BackendAgentNativeTimerCreateActivation.h"
#include "BackendAgentNativeTimerCreatePayload.h"
#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateDispatchService.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerSpecification.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{
using namespace vdrsuite::agent;
using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

NativeTimerSpecification timerSpecification(const std::string& suffix)
{
    NativeTimerSpecification value;
    value.channelId = "C-1-2-3";
    value.title = "Phase 64 activation " + suffix;
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

BackendAgentNativeTimerCreateSpecification agentSpecification(
    const std::string& suffix)
{
    const auto source = timerSpecification(suffix);
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

MutationOperation operation(const std::string& suffix)
{
    MutationOperation value;
    value.operationId = "op_create_activation_" + suffix;
    value.idempotencyKey = "idem_create_activation_" + suffix;
    value.actorId = "system_phase64";
    value.backendId = "default";
    value.backendGeneration = 7;
    value.resourceType = "TimerAssignment";
    value.resourceId = "ta_create_activation_" + suffix;
    value.expectedRevision = "4";
    value.expectedResourceFingerprint =
        nativeTimerSpecificationFingerprint(timerSpecification(suffix));
    value.actionFamily = "timer.create";
    value.requestFingerprint = "request_create_activation_" + suffix;
    value.requestedAt = 100;
    value.deadline = 500;
    value.verificationPolicy = MutationOperationVerificationPolicy::readbackRequired;
    value.state = MutationOperationState::accepted;
    value.updatedAt = 100;
    return value;
}

NativeTimerCreateOperationPayload operationPayload(const std::string& suffix)
{
    NativeTimerCreateOperationPayload value;
    value.timerAssignmentId = "ta_create_activation_" + suffix;
    value.expectedAssignmentRevision = "4";
    value.expectedIntentRevision = "9";
    value.assignmentEpoch = 3;
    value.nativeTimerBindingId = "ntb_create_activation_" + suffix;
    value.backendId = "default";
    value.backendGeneration = 7;
    value.expectedSpecification = timerSpecification(suffix);
    return value;
}

BackendAgentLocalProviderFacts providerFacts(
    const std::string& epoch,
    std::uint64_t generation,
    std::uint64_t capabilityRevision)
{
    BackendAgentLocalProviderFacts facts;
    facts.providerId = kBackendAgentNativeTimerCreateProviderId;
    facts.providerKind = kBackendAgentNativeTimerCreateProviderKind;
    facts.providerInstanceEpoch = epoch;
    facts.providerGeneration = generation;
    facts.capabilityRevision = capabilityRevision;
    facts.available = true;
    facts.capabilities = {kBackendAgentNativeTimerCreateCapability};
    return facts;
}

void observe(
    BackendAgentCommandRepository& commands,
    const BackendAgentLocalProviderFacts& facts,
    std::int64_t now)
{
    BackendAgentCommandPollRequest poll;
    poll.backendId = "default";
    poll.agentInstanceId = "inst_create_activation";
    poll.backendGeneration = 7;
    poll.supportedCommandTypes = {"probe.noop"};
    poll.localProviders = {facts};
    const auto observed = commands.poll(poll, "agt_create_activation", now);
    assert(observed.accepted);
    assert(!observed.assignment.present);
}

BackendAgentCommandAssignment assignment(
    const std::string& suffix,
    const BackendAgentLocalProviderSelection& selection)
{
    BackendAgentNativeTimerCreatePayload payload;
    payload.operationRevision = "1";
    payload.timerAssignmentId = "ta_create_activation_" + suffix;
    payload.expectedAssignmentRevision = "4";
    payload.expectedIntentRevision = "9";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "ntb_create_activation_" + suffix;
    payload.controlPlaneClaimedAt = 109;
    payload.specification = agentSpecification(suffix);
    payload.expectedSpecificationFingerprint =
        backendAgentNativeTimerCreateSpecificationFingerprint(payload.specification);
    payload.localProviderSelection = selection;

    BackendAgentCommandAssignment value;
    value.present = true;
    value.requestId = "req_create_activation_" + suffix;
    value.correlationId = value.requestId;
    value.operationId = "op_create_activation_" + suffix;
    value.jobId = "job_create_activation_" + suffix;
    value.attemptId = "attempt_create_activation_" + suffix;
    value.claimEpoch = 1;
    value.commandId = "cmd_create_activation_" + suffix;
    value.backendId = "default";
    value.agentId = "agt_create_activation";
    value.agentInstanceId = "inst_create_activation";
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

void persistOperation(
    MutationOperationRepository& operations,
    const std::string& suffix)
{
    const auto payload = operationPayload(suffix);
    MutationOperationPayload durable;
    durable.operationId = "op_create_activation_" + suffix;
    durable.payloadType = "native.timer.create";
    durable.payloadVersion = 1;
    durable.payload = serializeNativeTimerCreateOperationPayload(payload);
    durable.payloadFingerprint = nativeTimerCreateOperationPayloadFingerprint(payload);
    const auto reserved = operations.reserveWithPayload(operation(suffix), durable);
    assert(reserved.ok());
    assert(reserved.operation.operationRevision == "1");
}

void claimDispatching(
    MutationOperationRepository& operations,
    const std::string& suffix,
    const BackendAgentCommandAssignment& command)
{
    NativeTimerCreateDispatchClaimRequest request;
    request.operationId = "op_create_activation_" + suffix;
    request.expectedOperationRevision = "1";
    request.timerAssignmentId = "ta_create_activation_" + suffix;
    request.nativeTimerBindingId = "ntb_create_activation_" + suffix;
    request.backendId = "default";
    request.backendGeneration = 7;
    request.expectedSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(timerSpecification(suffix));
    request.reservation.commandId = command.commandId;
    request.reservation.requestFingerprint = command.requestFingerprint;
    NativeTimerCreateDispatchService dispatch(operations);
    const auto claimed = dispatch.claimAfterReservation(request, 120);
    assert(claimed.status == NativeTimerCreateDispatchClaimStatus::claimed);
    assert(claimed.operation.state == MutationOperationState::dispatching);
    assert(claimed.operation.operationRevision == "2");
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MutationOperationRepository operations(database);
    BackendAgentCommandRepository commands(database);
    BackendAgentCommandReservationRepository reservations(database);
    assert(operations.ensureSchema());
    assert(commands.ensureSchema());
    assert(reservations.ensureSchema());

    BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        "default", kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateProviderId,
        kBackendAgentNativeTimerCreateProviderKind,
        {kBackendAgentNativeTimerCreateCapability},
        100, ownership, reason));

    const auto firstFacts = providerFacts("pie_create_activation_1", 3, 4);
    observe(commands, firstFacts, 101);
    const auto firstSelection = commands.selectLocalProvider(
        "default", "agt_create_activation", "inst_create_activation", 7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability, reason);
    assert(firstSelection.has_value());

    persistOperation(operations, "1");
    const auto firstCommand = assignment("1", *firstSelection);
    const auto firstReservation = reservations.reserve(firstCommand, &*firstSelection);
    assert(firstReservation.ok());

    BackendAgentNativeTimerCreateActivationService activation(
        operations, reservations, commands);

    // A durable reservation alone is deliberately non-pollable and cannot be
    // activated before the MutationOperation has durably entered dispatching.
    const auto tooEarly = activation.activateDispatching(firstCommand.operationId);
    assert(tooEarly.status ==
        BackendAgentNativeTimerCreateActivationStatus::operationNotDispatching);
    assert(!commands.findAssignmentForOperation(
        "default", firstCommand.operationId,
        kBackendAgentNativeTimerCreateCommandType).has_value());

    claimDispatching(operations, "1", firstCommand);
    const auto activated = activation.activateDispatching(firstCommand.operationId);
    assert(activated.status == BackendAgentNativeTimerCreateActivationStatus::activated);
    assert(activated.assignment.commandId == firstCommand.commandId);
    assert(activated.assignment.requestFingerprint == firstCommand.requestFingerprint);

    const auto active = commands.findAssignmentForOperation(
        "default", firstCommand.operationId,
        kBackendAgentNativeTimerCreateCommandType);
    assert(active.has_value());
    assert(active->commandId == firstCommand.commandId);

    // Crash/restart after activation recovers the exact active reservation;
    // no new command/job/attempt identity is generated.
    const auto replay = activation.activateDispatching(firstCommand.operationId);
    assert(replay.status ==
        BackendAgentNativeTimerCreateActivationStatus::alreadyActivated);
    assert(replay.assignment.commandId == firstCommand.commandId);
    assert(replay.assignment.jobId == firstCommand.jobId);
    assert(replay.assignment.attemptId == firstCommand.attemptId);

    // A provider replacement after dispatching but before activation is a
    // fail-closed boundary. The operation remains dispatching and the original
    // reservation remains the only legal command identity.
    persistOperation(operations, "2");
    const auto secondCommand = assignment("2", *firstSelection);
    assert(reservations.reserve(secondCommand, &*firstSelection).ok());
    claimDispatching(operations, "2", secondCommand);
    observe(commands, providerFacts("pie_create_activation_2", 4, 5), 130);

    const auto stale = activation.activateDispatching(secondCommand.operationId);
    assert(stale.status ==
        BackendAgentNativeTimerCreateActivationStatus::providerSelectionStale);
    assert(!commands.findAssignmentForOperation(
        "default", secondCommand.operationId,
        kBackendAgentNativeTimerCreateCommandType).has_value());
    const auto stillReserved = reservations.findByCommandId(secondCommand.commandId);
    assert(stillReserved.ok());
    assert(stillReserved.reservation.assignment.requestFingerprint ==
        secondCommand.requestFingerprint);

    // Tampering with the durable dispatch reference never redirects activation
    // to another reservation.
    persistOperation(operations, "3");
    const auto currentSelection = commands.selectLocalProvider(
        "default", "agt_create_activation", "inst_create_activation", 7,
        kBackendAgentNativeTimerCreateAuthorityDomain,
        kBackendAgentNativeTimerCreateCapability, reason);
    assert(currentSelection.has_value());
    const auto thirdCommand = assignment("3", *currentSelection);
    assert(reservations.reserve(thirdCommand, &*currentSelection).ok());
    claimDispatching(operations, "3", thirdCommand);
    assert(database.execute(
        "UPDATE mutation_operations SET result_reference="
        "'native-timer-create-command-reservation/1|25:cmd_create_activation_other|20:fp1_0123456789abcdef|' "
        "WHERE operation_id='op_create_activation_3';"));
    const auto tampered = activation.activateDispatching(thirdCommand.operationId);
    assert(tampered.status ==
        BackendAgentNativeTimerCreateActivationStatus::dispatchReferenceInvalid ||
        tampered.status ==
            BackendAgentNativeTimerCreateActivationStatus::reservationNotFound);
    assert(!commands.findAssignmentForOperation(
        "default", thirdCommand.operationId,
        kBackendAgentNativeTimerCreateCommandType).has_value());

    return 0;
}
