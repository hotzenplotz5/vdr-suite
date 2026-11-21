#include "Database.h"
#include "MutationOperationRepository.h"
#include "NativeTimerCreateOperationPayload.h"
#include "NativeTimerCreateOperationPreparationService.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Managed Timer";
    value.directory = "VDR-Suite";
    value.day = "2026-08-17";
    value.weekdays = "-------";
    value.startTime = "930";
    value.endTime = "1015";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerIntent activeIntent(TimerIntentRepository& repository)
{
    TimerIntent intent;
    intent.timerIntentId = "intent:1";
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:create";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.canonicalChannelId = "channel:ard";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Berlin";
    intent.createdAt = 100;
    intent.updatedAt = 100;
    intent.expiresAt = 3000;
    const auto created = repository.create(intent);
    assert(created.ok());

    TimerIntent active = created.intent;
    active.state = TimerIntentState::active;
    active.updatedAt = 101;
    const auto updated = repository.update(active, created.intent.intentRevision);
    assert(updated.ok());
    return updated.intent;
}

TimerAssignment selectedAssignment(
    TimerAssignmentRepository& repository,
    const TimerIntent& intent)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:1";
    assignment.timerIntentId = intent.timerIntentId;
    assignment.intentRevision = intent.intentRevision;
    assignment.backendId = "backend:1";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:ard";
    assignment.channelBinding.backendChannelId = "S19.2E-1-1019-10301";
    assignment.channelBinding.mappingSource = "canonical-channel-map";
    assignment.channelBinding.mappingRevision = "mapping:7";
    assignment.capabilityRevision = "capability:7";
    assignment.backendHealthRevision = "health:7";
    assignment.decisionPolicyVersion = "policy:1";
    assignment.decisionEvidence.reasons = {"eligible_backend"};
    assignment.createdAt = 200;
    assignment.updatedAt = 200;
    const auto created = repository.create(assignment);
    assert(created.ok());
    return created.assignment;
}

TimerAssignment provisioningAssignment(
    TimerAssignmentRepository& repository,
    const TimerAssignment& selected)
{
    TimerAssignment next = selected;
    next.state = TimerAssignmentState::provisioning;
    next.updatedAt = 201;
    const auto updated = repository.update(next, selected.assignmentRevision);
    assert(updated.ok());
    return updated.assignment;
}

NativeTimerCreateOperationPreparationRequest requestFor(
    const TimerIntent& intent,
    const TimerAssignment& assignment,
    const std::string& operationId = "operation:create:1",
    const std::string& idempotencyKey = "idempotency:create:1")
{
    NativeTimerCreateOperationPreparationRequest request;
    request.operationId = operationId;
    request.idempotencyKey = idempotencyKey;
    request.actorId = "actor:owner";
    request.requestFingerprint = "request:create:fingerprint:1";
    request.timerAssignmentId = assignment.timerAssignmentId;
    request.expectedAssignmentRevision = assignment.assignmentRevision;
    request.expectedIntentRevision = intent.intentRevision;
    request.expectedAssignmentEpoch = assignment.assignmentEpoch;
    request.nativeTimerBindingId = "binding:reserved:1";
    request.expectedBackendId = assignment.backendId;
    request.expectedBackendGeneration = assignment.backendGeneration;
    request.expectedSpecification = specification();
    request.requestedAt = 300;
    request.deadline = 900;
    return request;
}

void assertPayloadCodec()
{
    NativeTimerCreateOperationPayload payload;
    payload.timerAssignmentId = "assignment:1";
    payload.expectedAssignmentRevision = "2";
    payload.expectedIntentRevision = "2";
    payload.assignmentEpoch = 3;
    payload.nativeTimerBindingId = "binding:reserved:1";
    payload.backendId = "backend:1";
    payload.backendGeneration = 7;
    payload.expectedSpecification = specification();

    assert(nativeTimerCreateOperationPayloadValid(payload));
    const std::string serialized =
        serializeNativeTimerCreateOperationPayload(payload);
    const std::string fingerprint =
        nativeTimerCreateOperationPayloadFingerprint(payload);
    assert(!serialized.empty());
    assert(!fingerprint.empty());

    NativeTimerCreateOperationPayload parsed;
    assert(parseNativeTimerCreateOperationPayload(serialized, parsed));
    assert(parsed.timerAssignmentId == payload.timerAssignmentId);
    assert(parsed.expectedAssignmentRevision == "2");
    assert(parsed.expectedIntentRevision == "2");
    assert(parsed.assignmentEpoch == 3);
    assert(parsed.nativeTimerBindingId == "binding:reserved:1");
    assert(parsed.backendId == "backend:1");
    assert(parsed.backendGeneration == 7);
    assert(parsed.expectedSpecification.startTime == "0930");
    assert(nativeTimerSpecificationFingerprint(parsed.expectedSpecification) ==
        nativeTimerSpecificationFingerprint(payload.expectedSpecification));

    auto padded = payload;
    padded.expectedSpecification.startTime = "0930";
    assert(serializeNativeTimerCreateOperationPayload(padded) == serialized);
    assert(nativeTimerCreateOperationPayloadFingerprint(padded) == fingerprint);

    assert(!parseNativeTimerCreateOperationPayload(
        serialized.substr(0, serialized.size() - 1), parsed));
    assert(!parseNativeTimerCreateOperationPayload(
        "native-timer-create-operation-payload/2|", parsed));
}
}

int main()
{
    assertPayloadCodec();

    Database database;
    assert(database.open(":memory:"));
    TimerIntentRepository intentRepository(database);
    TimerAssignmentRepository assignmentRepository(database);
    MutationOperationRepository operationRepository(database);
    assert(intentRepository.ensureSchema());
    assert(assignmentRepository.ensureSchema());
    assert(operationRepository.ensureSchema());

    const TimerIntent intent = activeIntent(intentRepository);
    const TimerAssignment selected = selectedAssignment(assignmentRepository, intent);

    NativeTimerCreateOperationPreparationService service(
        intentRepository, assignmentRepository, operationRepository);

    const auto selectedRequest = requestFor(intent, selected);
    assert(service.prepare(selectedRequest).status ==
        NativeTimerCreateOperationPreparationStatus::assignmentStateConflict);

    const TimerAssignment provisioning =
        provisioningAssignment(assignmentRepository, selected);
    const auto request = requestFor(intent, provisioning);

    const auto prepared = service.prepare(request);
    assert(prepared.status ==
        NativeTimerCreateOperationPreparationStatus::prepared);
    assert(prepared.operation.operationRevision == "1");
    assert(prepared.operation.state == MutationOperationState::accepted);
    assert(prepared.operation.resourceType == "TimerAssignment");
    assert(prepared.operation.resourceId == provisioning.timerAssignmentId);
    assert(prepared.operation.expectedRevision ==
        provisioning.assignmentRevision);
    assert(prepared.operation.expectedResourceFingerprint ==
        nativeTimerSpecificationFingerprint(request.expectedSpecification));
    assert(prepared.operation.actionFamily == "timer.create");
    assert(prepared.operation.backendId == "backend:1");
    assert(prepared.operation.backendGeneration == 7);
    assert(prepared.payload.nativeTimerBindingId == "binding:reserved:1");

    const auto durablePayload =
        operationRepository.findPayloadByOperationId(request.operationId);
    assert(durablePayload.ok());
    assert(durablePayload.payload.payloadType == "native.timer.create");
    assert(durablePayload.payload.payloadVersion == 1);
    assert(durablePayload.payload.payloadFingerprint ==
        nativeTimerCreateOperationPayloadFingerprint(prepared.payload));

    NativeTimerCreateOperationPayload recovered;
    assert(parseNativeTimerCreateOperationPayload(
        durablePayload.payload.payload, recovered));
    assert(recovered.timerAssignmentId == provisioning.timerAssignmentId);
    assert(recovered.expectedAssignmentRevision ==
        provisioning.assignmentRevision);
    assert(recovered.expectedIntentRevision == intent.intentRevision);
    assert(recovered.assignmentEpoch == provisioning.assignmentEpoch);
    assert(recovered.nativeTimerBindingId == "binding:reserved:1");
    assert(recovered.backendId == provisioning.backendId);
    assert(recovered.backendGeneration == provisioning.backendGeneration);

    // A new service instance can recover the exact immutable CREATE handoff.
    MutationOperationRepository restartedOperationRepository(database);
    NativeTimerCreateOperationPreparationService restartedService(
        intentRepository, assignmentRepository, restartedOperationRepository);
    const auto replay = restartedService.prepare(request);
    assert(replay.status ==
        NativeTimerCreateOperationPreparationStatus::alreadyPrepared);
    assert(replay.operation.operationRevision == "1");

    auto staleRevision = request;
    staleRevision.expectedAssignmentRevision = selected.assignmentRevision;
    assert(service.prepare(staleRevision).status ==
        NativeTimerCreateOperationPreparationStatus::assignmentRevisionConflict);

    auto staleIntent = request;
    staleIntent.expectedIntentRevision = "1";
    assert(service.prepare(staleIntent).status ==
        NativeTimerCreateOperationPreparationStatus::intentRevisionConflict);

    auto staleEpoch = request;
    ++staleEpoch.expectedAssignmentEpoch;
    assert(service.prepare(staleEpoch).status ==
        NativeTimerCreateOperationPreparationStatus::assignmentEpochConflict);

    auto wrongBackend = request;
    wrongBackend.expectedBackendId = "backend:other";
    assert(service.prepare(wrongBackend).status ==
        NativeTimerCreateOperationPreparationStatus::backendConflict);

    auto wrongGeneration = request;
    ++wrongGeneration.expectedBackendGeneration;
    assert(service.prepare(wrongGeneration).status ==
        NativeTimerCreateOperationPreparationStatus::generationConflict);

    auto wrongChannel = request;
    wrongChannel.expectedSpecification.channelId = "channel:wrong";
    assert(service.prepare(wrongChannel).status ==
        NativeTimerCreateOperationPreparationStatus::channelConflict);

    auto changedDesiredState = request;
    changedDesiredState.expectedSpecification.title = "Changed Timer";
    assert(service.prepare(changedDesiredState).status ==
        NativeTimerCreateOperationPreparationStatus::operationConflict);

    auto competingOperation = request;
    competingOperation.operationId = "operation:create:other";
    assert(service.prepare(competingOperation).status ==
        NativeTimerCreateOperationPreparationStatus::idempotencyConflict);

    const auto dispatching = operationRepository.transition(
        request.operationId,
        "1",
        MutationOperationState::accepted,
        MutationOperationState::dispatching,
        "",
        301);
    assert(dispatching.ok());
    assert(service.prepare(request).status ==
        NativeTimerCreateOperationPreparationStatus::operationStateConflict);

    std::cout << "test_native_timer_create_operation_preparation_service passed\n";
    return 0;
}
