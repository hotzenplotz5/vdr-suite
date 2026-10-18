#include "Database.h"
#include "EpgTimerIntentDraft.h"
#include "EpgTimerIntentAtomicAdmissionService.h"
#include "EpgNativeTimerClockAttestation.h"
#include "EpgNativeTimerPreviewAttestation.h"
#include "TimerIntentApplicationService.h"
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

void test_source_epg_draft_to_atomic_native_create_admission(bool rollbackWholeRequest)
{
    using namespace vdrsuite::public_epg_timer;
    Fixture fixture;
    const Selection chosen{"backend-one", "S19.2E-1-1019-10301", "81234",
        "\"opaque-epg-revision\"", "vidaa-epg-once"};
    AuthoritativeObservation source{chosen.backendId, chosen.channelId,
        chosen.eventId, "backend-one", chosen.ifMatch,
        1792222800, 1792226400, 7, true, true};
    ServerOwnedRecordingDefaults policy;
    policy.timezone = "Europe/Berlin";
    policy.channelSourceType = "vdr";
    const auto draft = makeProgrammeEventDraft(
        chosen, source, "actor:owner", policy, true, true, 1792223000);
    assert(draft.valid);
    assert(draft.specification.intentType == TimerIntentType::programmeEvent);
    assert(draft.specification.backendEventRef.eventId == chosen.eventId);

    TimerIntentApplicationRequest applicationRequest;
    applicationRequest.intent.timerIntentId = "intent:epg-native";
    applicationRequest.intent.state = TimerIntentState::draft;
    applicationRequest.intent.createdByActorId = "actor:owner";
    applicationRequest.intent.spec = draft.specification;
    applicationRequest.intent.createdAt = 1792223000;
    applicationRequest.intent.updatedAt = 1792223000;
    applicationRequest.intent.expiresAt = 1792320000;
    applicationRequest.timerAssignmentId = "assignment:epg-native";
    applicationRequest.activatedAt = 1792223001;
    applicationRequest.scheduledAt = 1792223002;
    applicationRequest.provisioningAt = 1792223003;

    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = chosen.backendId;
    candidate.siteId = "site:home";
    candidate.currentBackendGeneration = 7;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    candidate.executionAuthorityFence = "local:7";
    candidate.capability.backendGeneration = 7;
    candidate.capability.revision = "cap:7";
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;
    candidate.health.backendGeneration = 7;
    candidate.health.revision = "health:7";
    candidate.health.current = true;
    candidate.health.state = TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;
    candidate.channel.backendGeneration = 7;
    candidate.channel.mappingRevision = "mapping:7";
    candidate.channel.mappingSource = "native-source";
    candidate.channel.sourceType = "vdr";
    candidate.channel.sourceId = source.sourceId;
    candidate.channel.sourceChannelId = chosen.channelId;
    candidate.channel.backendChannelId = chosen.channelId;
    candidate.channel.current = true;
    candidate.desiredNativeTimerSpecificationPresent = true;
    candidate.desiredNativeTimerSpecification = specification();
    candidate.desiredNativeTimerSpecification.day = "2026-10-17";
    candidate.desiredNativeTimerSpecification.startTime = "0940";
    candidate.desiredNativeTimerSpecification.endTime = "1040";
    candidate.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    applicationRequest.candidates = {candidate};

    TimerIntentApplicationService application(
        fixture.intents, fixture.assignments, fixture.bindings);
    auto lease = fixture.database.acquireTransactionLease();
    assert(fixture.database.execute("BEGIN IMMEDIATE TRANSACTION;"));
    const auto selected =
        application.submitAndSelectPrimaryInCurrentTransaction(applicationRequest);
    assert(selected.status == TimerIntentApplicationStatus::assignmentSelected);
    assert(selected.assignment.state == TimerAssignmentState::selected);
    assert(selected.assignment.assignmentRevision == "1");
    assert(selected.assignment.backendGeneration == source.backendGeneration);
    assert(selected.assignment.desiredNativeTimerSpecificationPresent);
    const auto again =
        application.submitAndSelectPrimaryInCurrentTransaction(applicationRequest);
    assert(again.status == TimerIntentApplicationStatus::assignmentSelected);
    assert(again.assignment.assignmentRevision == "1");

    const auto absent = fixture.operations.findByIdempotencyScope(
        "actor:owner", chosen.backendId, "TimerAssignment",
        applicationRequest.timerAssignmentId, "timer.create",
        chosen.idempotencyKey);
    assert(absent.status == MutationOperationRepositoryStatus::notFound);

    NativeTimerCreateAdmissionRequest native;
    native.actorId = "actor:owner";
    native.idempotencyKey = chosen.idempotencyKey;
    native.timerAssignmentId = applicationRequest.timerAssignmentId;
    native.expectedAssignmentRevision = selected.assignment.assignmentRevision;
    native.expectedBackendId = chosen.backendId;
    native.requestedAt = 1792223003;
    native.deadline = 1792223300;
    const auto admitted =
        fixture.admission.admitInCurrentTransaction(native);
    assert(admitted.status == NativeTimerCreateAdmissionStatus::prepared);
    assert(admitted.operation.state == MutationOperationState::accepted);
    assert(admitted.operation.resourceId == applicationRequest.timerAssignmentId);
    assert(!admitted.payload.nativeTimerBindingId.empty());
    const auto stored = fixture.assignments.findById(
        applicationRequest.timerAssignmentId);
    assert(stored.ok());
    assert(stored.assignment.state == TimerAssignmentState::provisioning);
    assert(stored.assignment.assignmentRevision == "2");

    if (rollbackWholeRequest)
    {
        assert(fixture.database.execute("ROLLBACK;"));
        assert(fixture.intents.findById(
            applicationRequest.intent.timerIntentId).status ==
            TimerIntentRepositoryStatus::notFound);
        assert(fixture.assignments.findById(
            applicationRequest.timerAssignmentId).status ==
            TimerAssignmentRepositoryStatus::notFound);
        const auto absentAfterRollback = fixture.operations.findByIdempotencyScope(
            "actor:owner", chosen.backendId, "TimerAssignment",
            applicationRequest.timerAssignmentId, "timer.create",
            chosen.idempotencyKey);
        assert(absentAfterRollback.status ==
            MutationOperationRepositoryStatus::notFound);
        return;
    }

    assert(fixture.database.execute("COMMIT;"));
    const auto replay = fixture.admission.admit(native);
    assert(replay.status == NativeTimerCreateAdmissionStatus::replayed);
    assert(replay.operation.operationId == admitted.operation.operationId);
    source.exactNativeIdentity = false;
    assert(!makeProgrammeEventDraft(
        chosen, source, "actor:owner", policy, true, true, 1792223000).valid);
}

void test_epg_native_time_contract()
{
    using namespace vdrsuite::public_epg_timer;
    AuthoritativeObservation event;
    event.channelId = "S19.2E-1-1019-10301";
    event.startTime = 1792222800; // 2026-10-17 07:40 UTC
    event.endTime = 1792226400;
    event.exactNativeIdentity = true;
    event.observationCurrent = true;
    ServerOwnedRecordingDefaults defaults;
    defaults.timezone = "Europe/Berlin";
    defaults.priority = 50;
    defaults.lifetimeDays = 99;
    auto native = specification();
    native.channelId = event.channelId;
    native.day = "2026-10-17";
    native.startTime = "0940";
    native.endTime = "1040";
    const ServerOwnedNativeClockEvidence cest{true, 7200, 7200};
    assert(nativeOneShotMatchesEpg(event, defaults, cest, native));
    assert(!nativeOneShotMatchesEpg(event, defaults, {}, native));
    const ServerOwnedNativeClockEvidence wrongZone{true, 0, 0};
    assert(!nativeOneShotMatchesEpg(event, defaults, wrongZone, native));
    native.weekdays = "MTWTF--";
    assert(!nativeOneShotMatchesEpg(event, defaults, cest, native));
    native.weekdays = "-------";
    defaults.startMarginSeconds = 60;
    assert(!nativeOneShotMatchesEpg(event, defaults, cest, native));
    native.startTime = "0939";
    assert(nativeOneShotMatchesEpg(event, defaults, cest, native));
    defaults.startMarginSeconds = 0;

    // 2026-10-25: Europe's fall-back can give the two boundaries
    // different source-attested UTC offsets, never a single inferred one.
    event.startTime = 1792888200; // 2026-10-25 00:30 UTC
    event.endTime = 1792895400;   // 2026-10-25 02:30 UTC
    native.day = "2026-10-25";
    native.startTime = "0230";
    native.endTime = "0330";
    // A DST-fold crossing is currently unrepresentable in a native VDR
    // HHMM timer (no fold discriminator). No ambiguous automatic mutation.
    assert(!nativeOneShotMatchesEpg(event, defaults,
        ServerOwnedNativeClockEvidence{true, 7200, 3600}, native));
    assert(!nativeOneShotMatchesEpg(event, defaults,
        ServerOwnedNativeClockEvidence{true, 7200, 7200}, native));
    // End-of-program seconds round upwards, start seconds downwards.
    event.startTime = 1792888229;
    event.endTime = 1792895437;
    native.endTime = "0331";
    assert(!nativeOneShotMatchesEpg(event, defaults,
        ServerOwnedNativeClockEvidence{true, 7200, 3600}, native));
    // Normal unchanged-offset schedule still rounds deterministically.
    event.startTime = 1792222829;
    event.endTime = 1792226437;
    native.day = "2026-10-17";
    native.startTime = "0940";
    native.endTime = "1041";
    assert(nativeOneShotMatchesEpg(event, defaults,
        ServerOwnedNativeClockEvidence{true, 7200, 7200}, native));
    // A one-shot timer with HHMM start/end cannot encode an explicit
    // stop date beyond the following day; even exact 24h is ambiguous.
    defaults.startMarginSeconds = 24 * 60 * 60;
    native.day = "2026-10-16";
    native.startTime = "0940";
    native.endTime = "1040";
    assert(!nativeOneShotMatchesEpg(event, defaults, cest, native));
    defaults.startMarginSeconds = 0;

    // A normal midnight-crossing remains representable.
    event.startTime = 1792269600; // 2026-10-17 20:40 UTC / 22:40 CEST
    event.endTime = 1792276800;   // 2026-10-17 22:40 UTC / 00:40 CEST
    native.day = "2026-10-17";
    native.startTime = "2240";
    native.endTime = "0040";
    assert(nativeOneShotMatchesEpg(event, defaults, cest, native));
}

void test_epg_public_atomic_admission_and_replay()
{
    using namespace vdrsuite::public_epg_timer;
    Fixture fixture;
    TimerIntentApplicationService application(
        fixture.intents, fixture.assignments, fixture.bindings);
    EpgTimerIntentAtomicAdmissionService coordinator(
        fixture.database, fixture.intents, fixture.assignments,
        application, fixture.admission);
    assert(coordinator.ensureSchema());
    AtomicAdmissionRequest request;
    request.selection = {"backend-one", "S19.2E-1-1019-10301", "81234",
        "\"opaque-epg-v1\"", "device-once-1"};
    request.observation = {request.selection.backendId,
        request.selection.channelId, request.selection.eventId,
        "backend-one", request.selection.ifMatch,
        1792222800, 1792226400, 7U, true, true};
    request.defaults.timezone = "Europe/Berlin";
    request.defaults.channelSourceType = "vdr";
    request.authenticatedActorId = "actor:owner";
    request.authoritativeTitle = "Atomic admission";
    request.nativeClock = {true, 7200, 7200}; // attested backend local CEST
    request.requestedAt = 1792223000;
    request.deadline = 0;
    auto& candidate = request.candidate;
    candidate.backendId = "backend-one";
    candidate.siteId = "site:home";
    candidate.currentBackendGeneration = 7;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    request.authoritativeAgent.agentId = "agent:owner";
    request.authoritativeAgent.backendId = candidate.backendId;
    request.authoritativeAgent.agentInstanceId = "agent-instance:7";
    request.authoritativeAgent.backendGeneration = 7;
    request.authoritativeAgent.heartbeatSequence = 8;
    request.authoritativeAgent.capabilityRevision = 2;
    request.authoritativeAgent.lastHeartbeatAt = request.requestedAt - 5;
    request.authoritativeAgent.leaseExpiresAt = request.requestedAt + 30;
    auto& provider = request.nativeCreateProvider;
    provider.backendId = candidate.backendId;
    provider.authorityDomain = "vdr.timer";
    provider.providerId = "suitebridge:local";
    provider.providerKind = "suitebridge";
    provider.ownershipGeneration = 3;
    provider.providerInstanceEpoch = "pie_9a2839a085848a27bd72c66481547420";
    provider.providerGeneration = 1;
    provider.capabilityRevision = 1;
    provider.requiredCapability = "vdr.timer.create";
    candidate.executionAuthorityFence =
        vdrsuite::public_epg_timer::nativeTimerCreatePlanningAuthorityFence(provider);
    // Already typed/verified backend projection is bound again to Agent
    // generation, Provider epoch, and the exact requested clock window.
    vdrsuite::agent::SuiteBridgeNativeTimerClockProjection projection;
    projection.start = request.observation.startTime;
    projection.stop = request.observation.endTime;
    projection.startUtcOffsetSeconds = 7200;
    projection.stopUtcOffsetSeconds = 7200;
    projection.startDay = "2026-10-17";
    projection.startHhmm = "0940";
    projection.stopDay = "2026-10-17";
    projection.stopHhmm = "1040";
    projection.providerInstanceEpoch = provider.providerInstanceEpoch;
    assert(attestBackendNativeTimerClock(
        projection, request.authoritativeAgent, provider, candidate,
        projection.start, projection.stop, request.requestedAt - 1,
        request.nativeClock));

    auto wrongClockReply = projection;
    wrongClockReply.startHhmm = "0840";
    ServerOwnedNativeClockEvidence refusedClock;
    assert(!attestBackendNativeTimerClock(
        wrongClockReply, request.authoritativeAgent, provider, candidate,
        projection.start, projection.stop, request.requestedAt - 1,
        refusedClock));
    assert(!refusedClock.current);

    wrongClockReply = projection;
    wrongClockReply.providerInstanceEpoch = "pie_restarted";
    assert(!attestBackendNativeTimerClock(
        wrongClockReply, request.authoritativeAgent, provider, candidate,
        projection.start, projection.stop, request.requestedAt - 1,
        refusedClock));
    assert(!refusedClock.current);

    auto foreignClockAgent = request.authoritativeAgent;
    foreignClockAgent.backendGeneration++;
    assert(!attestBackendNativeTimerClock(
        projection, foreignClockAgent, provider, candidate,
        projection.start, projection.stop, request.requestedAt - 1,
        refusedClock));
    assert(!refusedClock.current);

    assert(!attestBackendNativeTimerClock(
        projection, request.authoritativeAgent, provider, candidate,
        projection.start + 60, projection.stop, request.requestedAt - 1,
        refusedClock));
    assert(!refusedClock.current);
    candidate.capability.backendGeneration = 7;
    candidate.capability.revision = "capability:7";
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;
    candidate.health.backendGeneration = 7;
    candidate.health.revision = "health:7";
    candidate.health.current = true;
    candidate.health.state = TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;
    candidate.channel.backendGeneration = 7;
    candidate.channel.mappingRevision = "native:7";
    candidate.channel.mappingSource = "native-epg";
    candidate.channel.sourceType = "vdr";
    candidate.channel.sourceId = "backend-one";
    candidate.channel.sourceChannelId = request.selection.channelId;
    candidate.channel.backendChannelId = request.selection.channelId;
    candidate.channel.current = true;
    candidate.desiredNativeTimerSpecificationPresent = true;
    candidate.desiredNativeTimerSpecification = specification();
    candidate.desiredNativeTimerSpecification.day = "2026-10-17";
    candidate.desiredNativeTimerSpecification.startTime = "0940";
    candidate.desiredNativeTimerSpecification.endTime = "1040";
    candidate.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    // A producer-independent domain test: the typed projection is a fixture,
    // NOT live VDR evidence. Only the real native NTPREVIEW transport may
    // provide it in the productive Daemon.
    vdrsuite::agent::SuiteBridgeNativeTimerPreviewProjection preview;
    preview.isolatedCandidateClear = true;
    preview.proposedStart = request.observation.startTime;
    preview.proposedStop = request.observation.endTime;
    preview.nativeObservedAt = request.requestedAt - 1;
    preview.providerInstanceEpoch = provider.providerInstanceEpoch;
    preview.specificationDigest =
        vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
            nativeTimerSpecificationFingerprint(
                candidate.desiredNativeTimerSpecification));
    preview.evidenceRevision =
        vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
            "regression-native-preview-snapshot");
    preview.consideredTimers = 2;
    preview.consideredDevices = 1;
    {
        auto producerCandidate = candidate;
        ProspectiveNativeTimerConflictProof attested;
        assert(attestBackendNativeTimerPreview(
            preview, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(attested.current &&
            attested.nativeSpecificationFingerprint ==
                nativeTimerSpecificationFingerprint(
                    candidate.desiredNativeTimerSpecification));
        assert(producerCandidate.conflict ==
            TimerAssignmentPlanningConflictState::confirmedClear);
        assert(attested.observedAt == preview.nativeObservedAt);
        assert(attested.expiresAt == preview.nativeObservedAt + 15);

        // A native snapshot at the freshness boundary may be admitted now,
        // but its lifetime must NOT be renewed when the reply is received.
        auto lastFresh = preview;
        lastFresh.nativeObservedAt = request.requestedAt - 15;
        assert(attestBackendNativeTimerPreview(
            lastFresh, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(attested.observedAt == lastFresh.nativeObservedAt);
        assert(attested.expiresAt == request.requestedAt);
        assert(!prospectiveNativeTimerConflictClear(
            attested, producerCandidate, request.requestedAt + 1));

        auto stale = preview;
        stale.providerInstanceEpoch = "pie_stale";
        assert(!attestBackendNativeTimerPreview(
            stale, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(!attested.current);
        assert(producerCandidate.conflict ==
            TimerAssignmentPlanningConflictState::unavailable);

        stale = preview;
        stale.consideredDevices = 0;
        assert(!attestBackendNativeTimerPreview(
            stale, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(!attested.current);

        stale = preview;
        stale.specificationDigest = "sha256:wrong";
        assert(!attestBackendNativeTimerPreview(
            stale, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(!attested.current);

        stale = preview;
        stale.nativeObservedAt = request.requestedAt - 16;
        assert(!attestBackendNativeTimerPreview(
            stale, request.authoritativeAgent, provider,
            producerCandidate, request.requestedAt, attested));
        assert(!attested.current);

        auto unleased = request.authoritativeAgent;
        unleased.leaseExpiresAt = request.requestedAt;
        assert(!attestBackendNativeTimerPreview(
            preview, unleased, provider, producerCandidate,
            request.requestedAt, attested));
        assert(!attested.current);
    }
    request.conflictProof.current = true;
    request.conflictProof.backendId = candidate.backendId;
    request.conflictProof.backendGeneration = candidate.currentBackendGeneration;
    request.conflictProof.executionAuthorityFence = candidate.executionAuthorityFence;
    request.conflictProof.nativeSpecificationFingerprint =
        nativeTimerSpecificationFingerprint(candidate.desiredNativeTimerSpecification);
    request.conflictProof.evidenceRevision = "proposed-native-timer:7";
    request.conflictProof.observedAt = request.requestedAt - 1;
    request.conflictProof.expiresAt = request.requestedAt + 10;
    request.conflictProof.proposedTimerIncluded = true;
    request.conflictProof.allExistingTimersConsidered = true;
    request.conflictProof.devicesConsidered = true;
    request.conflictProof.conflictsFound = false;
    request.revalidateSourceAndNativeSpecification = [] { return true; };

    const auto first = coordinator.admit(request);
    assert(first.status == AtomicAdmissionStatus::prepared);
    assert(mutationOperationIdCanonical(first.operation.operationId));
    assert(first.operation.state == MutationOperationState::accepted);
    assert(first.operation.backendId == "backend-one");
    assert(fixture.database.transactionActive() == false);

    EpgTimerIntentAtomicAdmissionService afterRestart(
        fixture.database, fixture.intents, fixture.assignments,
        application, fixture.admission);
    const auto replay = afterRestart.admit(request);
    assert(replay.status == AtomicAdmissionStatus::replayed);
    assert(replay.operation.operationId == first.operation.operationId);
    assert(replay.operation.operationRevision == first.operation.operationRevision);

    // A lost first response can be replayed after the airing ends or the
    // Agent is disconnected. No new write / fresh authority is attempted.
    auto afterAiring = request;
    afterAiring.requestedAt = request.observation.endTime + 100;
    afterAiring.candidate = {};
    afterAiring.revalidateSourceAndNativeSpecification = {};
    const auto lateReplay = afterRestart.admit(afterAiring);
    assert(lateReplay.status == AtomicAdmissionStatus::replayed);
    assert(lateReplay.operation.operationId == first.operation.operationId);

    auto different = request;
    different.selection.eventId = "81235";
    different.observation.nativeEventId = "81235";
    const auto conflict = afterRestart.admit(different);
    assert(conflict.status == AtomicAdmissionStatus::idempotencyConflict);

    auto freshKey = request;
    freshKey.selection.idempotencyKey = "device-once-2";
    const auto duplicate = afterRestart.admit(freshKey);
    assert(duplicate.status == AtomicAdmissionStatus::duplicateIntent);

    auto absentPreview = request;
    absentPreview.selection.idempotencyKey = "device-no-native-preview";
    absentPreview.conflictProof.current = false;
    assert(afterRestart.admit(absentPreview).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto lsccPretend = request;
    lsccPretend.selection.idempotencyKey = "device-lscc-empty-not-proof";
    lsccPretend.conflictProof.proposedTimerIncluded = false;
    assert(afterRestart.admit(lsccPretend).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto noDeviceEvidence = request;
    noDeviceEvidence.selection.idempotencyKey = "device-no-device-evidence";
    noDeviceEvidence.conflictProof.devicesConsidered = false;
    assert(afterRestart.admit(noDeviceEvidence).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto stalePreview = request;
    stalePreview.selection.idempotencyKey = "device-stale-preview";
    stalePreview.conflictProof.observedAt -= 60;
    assert(afterRestart.admit(stalePreview).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto wrongFingerprint = request;
    wrongFingerprint.selection.idempotencyKey = "device-wrong-preview-spec";
    wrongFingerprint.conflictProof.nativeSpecificationFingerprint += "changed";
    assert(afterRestart.admit(wrongFingerprint).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto staleAgent = request;
    staleAgent.selection.idempotencyKey = "device-agent-lease-stale";
    staleAgent.authoritativeAgent.leaseExpiresAt = request.requestedAt;
    assert(afterRestart.admit(staleAgent).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto otherProviderEpoch = request;
    otherProviderEpoch.selection.idempotencyKey = "device-provider-restarted";
    otherProviderEpoch.nativeCreateProvider.providerInstanceEpoch = "pie_changed";
    assert(afterRestart.admit(otherProviderEpoch).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto wrongProviderKind = request;
    wrongProviderKind.selection.idempotencyKey = "device-provider-fallback";
    wrongProviderKind.nativeCreateProvider.providerKind = "restfulapi";
    assert(afterRestart.admit(wrongProviderKind).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto wrongTime = request;
    wrongTime.selection.idempotencyKey = "device-new-wrong-hour";
    wrongTime.candidate.desiredNativeTimerSpecification.startTime = "0840";
    assert(afterRestart.admit(wrongTime).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto oldClock = request;
    oldClock.selection.idempotencyKey = "device-new-old-clock";
    oldClock.nativeClock.observedAt -= 60;
    assert(afterRestart.admit(oldClock).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto restartedClockProvider = request;
    restartedClockProvider.selection.idempotencyKey = "device-new-clock-provider";
    restartedClockProvider.nativeClock.providerInstanceEpoch = "pie_old";
    assert(afterRestart.admit(restartedClockProvider).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto missingClock = request;
    missingClock.selection.idempotencyKey = "device-new-missing-clock";
    missingClock.nativeClock.current = false;
    assert(afterRestart.admit(missingClock).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto recurring = request;
    recurring.selection.idempotencyKey = "device-new-recurring";
    recurring.candidate.desiredNativeTimerSpecification.weekdays = "MTWTF--";
    assert(afterRestart.admit(recurring).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto generation = request;
    generation.selection.idempotencyKey = "device-new-stale-generation";
    generation.candidate.currentBackendGeneration = 8;
    assert(afterRestart.admit(generation).status ==
        AtomicAdmissionStatus::backendUnavailable);

    auto staleDuringAdmission = request;
    staleDuringAdmission.selection.idempotencyKey = "device-new-stale-authority";
    staleDuringAdmission.revalidateSourceAndNativeSpecification =
        [] { return false; };
    assert(afterRestart.admit(staleDuringAdmission).status ==
        AtomicAdmissionStatus::generationConflict);
    auto missingAuthority = request;
    missingAuthority.selection.idempotencyKey = "device-new-missing-authority";
    missingAuthority.revalidateSourceAndNativeSpecification = {};
    assert(afterRestart.admit(missingAuthority).status ==
        AtomicAdmissionStatus::backendUnavailable);

    // Failure after intent and assignment INSERT must leave neither one
    // committed, and must not reserve an Operation/idempotency alias.
    Fixture aborted;
    TimerIntentApplicationService abortedApplication(
        aborted.intents, aborted.assignments, aborted.bindings);
    EpgTimerIntentAtomicAdmissionService abortCoordinator(
        aborted.database, aborted.intents, aborted.assignments,
        abortedApplication, aborted.admission);
    assert(abortCoordinator.ensureSchema());
    assert(aborted.database.execute(
        "CREATE TRIGGER fail_epg_atomic_op "
        "BEFORE INSERT ON mutation_operations "
        "BEGIN SELECT RAISE(ABORT,'forced failure'); END;"));
    assert(abortCoordinator.admit(request).status ==
        AtomicAdmissionStatus::storageError);
    const auto expected = makeProgrammeEventDraft(
        request.selection, request.observation, request.authenticatedActorId,
        request.defaults, true, true, request.requestedAt);
    assert(expected.valid);
    const auto leftovers = aborted.intents.findEquivalent(expected.specification);
    assert(leftovers.ok() && leftovers.intents.empty());
    assert(aborted.database.execute("DROP TRIGGER fail_epg_atomic_op;"));
    const auto recovered = abortCoordinator.admit(request);
    assert(recovered.status == AtomicAdmissionStatus::prepared);
}

} // namespace

int main()
{
    test_source_epg_draft_to_atomic_native_create_admission(false);
    test_source_epg_draft_to_atomic_native_create_admission(true);
    test_epg_native_time_contract();
    test_epg_public_atomic_admission_and_replay();
    {
        Fixture fixture;
        // Nested admission without the coordinator's active transaction
        // must fail closed and leave all domain rows untouched.
        assert(fixture.admission.admitInCurrentTransaction(request()).status ==
            NativeTimerCreateAdmissionStatus::repositoryError);
        auto lease = fixture.database.acquireTransactionLease();
        assert(fixture.database.execute("BEGIN IMMEDIATE TRANSACTION;"));
        const auto nested =
            fixture.admission.admitInCurrentTransaction(request());
        assert(nested.status == NativeTimerCreateAdmissionStatus::prepared);
        assert(fixture.database.transactionActive());
        const auto pending = fixture.operations.findByIdempotencyScope(
            "actor:owner", "backend-one", "TimerAssignment",
            "assignment:atomic", "timer.create", "idem-atomic-1");
        assert(pending.ok());
        // Simulate a failed higher-level EPG admission after native
        // preparation: neither the Operation nor provisioning may survive.
        assert(fixture.database.execute("ROLLBACK;"));
        const auto vanished = fixture.operations.findByIdempotencyScope(
            "actor:owner", "backend-one", "TimerAssignment",
            "assignment:atomic", "timer.create", "idem-atomic-1");
        assert(vanished.status == MutationOperationRepositoryStatus::notFound);
        const auto restored = fixture.assignments.findById("assignment:atomic");
        assert(restored.ok());
        assert(restored.assignment.state == TimerAssignmentState::selected);
        assert(restored.assignment.assignmentRevision == "1");
        // Existing independent CREATE semantics remain unchanged.
        const auto accepted = fixture.admission.admit(request());
        assert(accepted.status == NativeTimerCreateAdmissionStatus::prepared);
    }
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
