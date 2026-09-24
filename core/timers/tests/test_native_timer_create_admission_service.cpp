#include "Database.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"
#include "NativeTimerBindingIdentity.h"
#include "NativeTimerBindingRepository.h"
#include "NativeTimerCreateAdmissionService.h"
#include "NativeTimerCreateOperationPreparationService.h"
#include "TimerAssignmentFulfillmentService.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <string>

using namespace vdrsuite::operations;
using namespace vdrsuite::timers;

namespace
{
NativeTimerSpecification specification()
{
    NativeTimerSpecification value;
    value.channelId = "S19.2E-1-1019-10301";
    value.title = "Atomic admission";
    value.directory = "VDR-Suite";
    value.day = "2026-09-24";
    value.weekdays = "-------";
    value.startTime = "0705";
    value.endTime = "0800";
    value.priority = 50;
    value.lifetime = 99;
    value.enabled = true;
    value.vps = false;
    return value;
}

TimerIntent createActiveIntent(TimerIntentRepository& repository)
{
    TimerIntent intent;
    intent.timerIntentId = "intent:atomic";
    intent.state = TimerIntentState::draft;
    intent.createdByActorId = "actor:creator";
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
    const auto updated =
        repository.update(active, created.intent.intentRevision);
    assert(updated.ok());
    return updated.intent;
}

TimerAssignment createSelectedAssignment(
    TimerAssignmentRepository& repository,
    const TimerIntent& intent)
{
    TimerAssignment assignment;
    assignment.timerAssignmentId = "assignment:atomic";
    assignment.timerIntentId = intent.timerIntentId;
    assignment.intentRevision = intent.intentRevision;
    assignment.backendId = "backend-one";
    assignment.backendGeneration = 7;
    assignment.state = TimerAssignmentState::selected;
    assignment.role = TimerAssignmentRole::primary;
    assignment.channelBinding.canonicalChannelId = "channel:ard";
    assignment.channelBinding.backendChannelId =
        "S19.2E-1-1019-10301";
    assignment.channelBinding.mappingSource =
        "canonical-channel-map";
    assignment.channelBinding.mappingRevision = "mapping:7";
    assignment.capabilityRevision = "capability:7";
    assignment.backendHealthRevision = "health:7";
    assignment.decisionPolicyVersion =
        "timer-assignment-planner/1";
    assignment.decisionEvidence.reasons = {
        "selected_eligible_backend"};
    assignment.desiredNativeTimerSpecificationPresent = true;
    assignment.desiredNativeTimerSpecification = specification();
    assignment.createdAt = 200;
    assignment.updatedAt = 200;

    const auto created = repository.create(assignment);
    assert(created.ok());
    return created.assignment;
}

NativeTimerCreateAdmissionRequest request(
    const std::string& key = "idem-atomic-1")
{
    NativeTimerCreateAdmissionRequest value;
    value.actorId = "actor:owner";
    value.idempotencyKey = key;
    value.timerAssignmentId = "assignment:atomic";
    value.expectedAssignmentRevision = "1";
    value.expectedBackendId = "backend-one";
    value.requestedAt = 200;
    value.deadline = 800;
    return value;
}

struct Fixture
{
    Database database;
    TimerIntentRepository intents;
    TimerAssignmentRepository assignments;
    NativeTimerBindingRepository bindings;
    MutationOperationRepository operations;
    TimerAssignmentFulfillmentService fulfillment;
    NativeTimerCreateOperationPreparationService preparation;
    NativeTimerCreateAdmissionService admission;

    Fixture()
        : intents(database),
          assignments(database),
          bindings(database),
          operations(database),
          fulfillment(assignments, bindings),
          preparation(intents, assignments, operations),
          admission(
              database,
              assignments,
              operations,
              fulfillment,
              preparation)
    {
        assert(database.open(":memory:"));
        assert(intents.ensureSchema());
        assert(assignments.ensureSchema());
        assert(bindings.ensureSchema());
        assert(operations.ensureSchema());
        const TimerIntent intent = createActiveIntent(intents);
        const TimerAssignment assignment =
            createSelectedAssignment(assignments, intent);
        assert(assignment.assignmentRevision == "1");
        assert(assignment.state == TimerAssignmentState::selected);
    }
};
} // namespace

int main()
{
    {
        Fixture fixture;

        const auto first = fixture.admission.admit(request());
        assert(first.status == NativeTimerCreateAdmissionStatus::prepared);
        assert(mutationOperationIdCanonical(first.operation.operationId));
        assert(nativeTimerBindingIdCanonical(
            first.payload.nativeTimerBindingId));
        assert(first.operation.operationRevision == "1");
        assert(first.operation.state == MutationOperationState::accepted);
        assert(first.operation.actorId == "actor:owner");
        assert(first.operation.idempotencyKey == "idem-atomic-1");
        assert(first.operation.backendId == "backend-one");
        assert(first.operation.resourceType == "TimerAssignment");
        assert(first.operation.resourceId == "assignment:atomic");
        assert(first.operation.actionFamily == "timer.create");
        assert(first.operation.expectedRevision == "2");
        assert(first.operation.requestFingerprint.find(
            "native-timer-create-admission/1|") == 0);

        const auto durableAssignment =
            fixture.assignments.findById("assignment:atomic");
        assert(durableAssignment.ok());
        assert(durableAssignment.assignment.state ==
            TimerAssignmentState::provisioning);
        assert(durableAssignment.assignment.assignmentRevision == "2");

        const auto durablePayload =
            fixture.operations.findPayloadByOperationId(
                first.operation.operationId);
        assert(durablePayload.ok());

        // The client may repeat the original selected-resource precondition.
        // Replay is resolved from the durable idempotency scope before looking
        // at the now-provisioning assignment, so no new IDs or revision are made.
        const auto replay = fixture.admission.admit(request());
        assert(replay.status == NativeTimerCreateAdmissionStatus::replayed);
        assert(replay.operation.operationId == first.operation.operationId);
        assert(replay.payload.nativeTimerBindingId ==
            first.payload.nativeTimerBindingId);
        const auto afterReplay =
            fixture.assignments.findById("assignment:atomic");
        assert(afterReplay.ok());
        assert(afterReplay.assignment.assignmentRevision == "2");

        auto changedPrecondition = request();
        changedPrecondition.expectedAssignmentRevision = "9";
        const auto idempotencyConflict =
            fixture.admission.admit(changedPrecondition);
        assert(idempotencyConflict.status ==
            NativeTimerCreateAdmissionStatus::idempotencyConflict);

        const auto staleNewLogicalRequest =
            fixture.admission.admit(request("idem-atomic-2"));
        assert(staleNewLogicalRequest.status ==
            NativeTimerCreateAdmissionStatus::assignmentRevisionConflict);
    }

    {
        Fixture fixture;

        // Force operation reservation to fail only after the fulfillment owner
        // has issued the provisioning update inside the same outer transaction.
        assert(fixture.database.execute(
            "CREATE TRIGGER fail_atomic_timer_create_operation "
            "BEFORE INSERT ON mutation_operations "
            "BEGIN SELECT RAISE(ABORT,'forced admission failure'); END;"));

        const auto failed = fixture.admission.admit(request());
        assert(failed.status ==
            NativeTimerCreateAdmissionStatus::repositoryError);

        // The assignment transition and operation/payload reservation are one
        // commit boundary: failure leaves the original selected row untouched.
        const auto selected =
            fixture.assignments.findById("assignment:atomic");
        assert(selected.ok());
        assert(selected.assignment.state == TimerAssignmentState::selected);
        assert(selected.assignment.assignmentRevision == "1");

        const auto absent =
            fixture.operations.findByIdempotencyScope(
                "actor:owner",
                "backend-one",
                "TimerAssignment",
                "assignment:atomic",
                "timer.create",
                "idem-atomic-1");
        assert(absent.status == MutationOperationRepositoryStatus::notFound);

        assert(fixture.database.execute(
            "DROP TRIGGER fail_atomic_timer_create_operation;"));

        const auto retry = fixture.admission.admit(request());
        assert(retry.status == NativeTimerCreateAdmissionStatus::prepared);
        const auto provisioned =
            fixture.assignments.findById("assignment:atomic");
        assert(provisioned.ok());
        assert(provisioned.assignment.state ==
            TimerAssignmentState::provisioning);
        assert(provisioned.assignment.assignmentRevision == "2");
    }

    {
        Fixture fixture;
        auto wrongBackend = request();
        wrongBackend.expectedBackendId = "backend-two";
        assert(fixture.admission.admit(wrongBackend).status ==
            NativeTimerCreateAdmissionStatus::assignmentNotFound);

        auto missingRevision = request();
        missingRevision.expectedAssignmentRevision.clear();
        assert(fixture.admission.admit(missingRevision).status ==
            NativeTimerCreateAdmissionStatus::invalid);
    }

    return 0;
}
