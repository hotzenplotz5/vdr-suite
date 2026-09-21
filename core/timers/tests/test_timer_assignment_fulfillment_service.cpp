#include "Database.h"
#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
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

NativeTimerBinding verifiedBinding(
    NativeTimerBindingRepository& repository,
    const TimerAssignment& assignment,
    const std::string& bindingId = "binding:1")
{
    NativeTimerBinding binding;
    binding.nativeTimerBindingId = bindingId;
    binding.backendId = assignment.backendId;
    binding.backendGeneration = assignment.backendGeneration;
    binding.backendNativeTimerId = "timer:17";
    binding.timerAssignmentId = assignment.timerAssignmentId;
    binding.ownership = NativeTimerBindingOwnership::managed;
    binding.observedState.channelId = assignment.channelBinding.backendChannelId;
    binding.observedState.title = "Managed Timer";
    binding.observedState.day = "2026-08-17";
    binding.observedState.weekdays = "-------";
    binding.observedState.startTime = "0930";
    binding.observedState.endTime = "1015";
    binding.observedFingerprint =
        nativeTimerObservedStateFingerprint(binding.observedState);
    binding.lastObservedAt = 210;
    binding.lastVerifiedOperationId = "operation:1";
    const auto created = repository.create(binding);
    assert(created.ok());
    return created.binding;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    TimerIntentRepository intentRepository(database);
    TimerAssignmentRepository assignmentRepository(database);
    NativeTimerBindingRepository bindingRepository(database);
    assert(intentRepository.ensureSchema());
    assert(assignmentRepository.ensureSchema());
    assert(bindingRepository.ensureSchema());

    const TimerIntent intent = activeIntent(intentRepository);
    const TimerAssignment selected = selectedAssignment(assignmentRepository, intent);
    TimerAssignmentFulfillmentService service(
        assignmentRepository, bindingRepository);

    const auto started = service.beginProvisioning(
        selected.timerAssignmentId,
        selected.assignmentRevision,
        intent.intentRevision,
        7,
        201);
    assert(started.status ==
        TimerAssignmentFulfillmentStatus::provisioningStarted);
    assert(started.assignment.state == TimerAssignmentState::provisioning);
    assert(started.assignment.assignmentRevision == "2");
    assert(started.assignment.nativeTimerBindingId.empty());

    const auto alreadyStarting = service.beginProvisioning(
        selected.timerAssignmentId,
        started.assignment.assignmentRevision,
        intent.intentRevision,
        7,
        202);
    assert(alreadyStarting.status ==
        TimerAssignmentFulfillmentStatus::alreadyProvisioning);

    const auto staleStart = service.beginProvisioning(
        selected.timerAssignmentId,
        selected.assignmentRevision,
        intent.intentRevision,
        7,
        202);
    assert(staleStart.status ==
        TimerAssignmentFulfillmentStatus::assignmentRevisionConflict);

    const NativeTimerBinding binding = verifiedBinding(
        bindingRepository, started.assignment);

    const auto bound = service.bindVerified(
        selected.timerAssignmentId,
        started.assignment.assignmentRevision,
        intent.intentRevision,
        7,
        binding.nativeTimerBindingId,
        binding.bindingRevision,
        211);
    assert(bound.status == TimerAssignmentFulfillmentStatus::bound);
    assert(bound.assignment.state == TimerAssignmentState::bound);
    assert(bound.assignment.assignmentRevision == "3");
    assert(bound.assignment.nativeTimerBindingId == "binding:1");

    const auto replay = service.bindVerified(
        selected.timerAssignmentId,
        started.assignment.assignmentRevision,
        intent.intentRevision,
        7,
        binding.nativeTimerBindingId,
        binding.bindingRevision,
        212);
    assert(replay.status == TimerAssignmentFulfillmentStatus::alreadyBound);
    assert(replay.assignment.assignmentRevision == "3");

    const auto wrongGeneration = service.bindVerified(
        selected.timerAssignmentId,
        bound.assignment.assignmentRevision,
        intent.intentRevision,
        8,
        binding.nativeTimerBindingId,
        binding.bindingRevision,
        213);
    assert(wrongGeneration.status ==
        TimerAssignmentFulfillmentStatus::generationConflict);

    const auto missingBinding = service.bindVerified(
        selected.timerAssignmentId,
        bound.assignment.assignmentRevision,
        intent.intentRevision,
        7,
        "binding:missing",
        "1",
        213);
    assert(missingBinding.status ==
        TimerAssignmentFulfillmentStatus::bindingNotFound);

    std::cout << "test_timer_assignment_fulfillment_service passed\n";
    return 0;
}
