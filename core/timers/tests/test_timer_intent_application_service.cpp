#include "Database.h"
#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentApplicationService.h"
#include "ManualTimerIntentDraft.h"
#include "TimerIntentRepository.h"

#include <cassert>
#include <iostream>

using namespace vdrsuite::timers;

namespace
{
TimerIntent intent()
{
    TimerIntent value;
    value.timerIntentId = "intent:application:1";
    value.state = TimerIntentState::draft;
    value.createdByActorId = "actor:create";
    value.spec.intentType = TimerIntentType::manualWindow;
    value.spec.ownerActorId = "actor:owner";
    value.spec.channelRequirement.canonicalChannelId = "channel:ard";
    value.spec.schedule.startAt = 1000;
    value.spec.schedule.stopAt = 2000;
    value.spec.schedule.timezone = "Europe/Berlin";
    value.spec.assignmentPolicy.preferredBackendIds = {"backend:alpha"};
    value.createdAt = 100;
    value.updatedAt = 100;
    value.expiresAt = 3000;
    return value;
}

TimerAssignmentPlanningBackendCandidate candidate()
{
    TimerAssignmentPlanningBackendCandidate value;
    value.backendId = "backend:alpha";
    value.siteId = "site:home";
    value.currentBackendGeneration = 7;
    value.state = TimerAssignmentPlanningBackendState::online;
    value.writeAllowed = true;
    value.executionAuthorityCurrent = true;
    value.executionAuthorityFence = "authority:7";
    value.capability.backendGeneration = 7;
    value.capability.revision = "capability:7";
    value.capability.current = true;
    value.capability.timerCreate = true;
    value.capability.timerReadback = true;
    value.health.backendGeneration = 7;
    value.health.revision = "health:7";
    value.health.current = true;
    value.health.state = TimerAssignmentPlanningHealthState::healthy;
    value.health.timerWritesAvailable = true;
    value.channel.backendGeneration = 7;
    value.channel.mappingRevision = "mapping:7";
    value.channel.mappingSource = "canonical-channel-map";
    value.channel.canonicalChannelId = "channel:ard";
    value.channel.backendChannelId = "S19.2E-1-1019-10301";
    value.channel.current = true;
    value.desiredNativeTimerSpecificationPresent = true;
    value.desiredNativeTimerSpecification.channelId =
        value.channel.backendChannelId;
    value.desiredNativeTimerSpecification.title = "Application";
    value.desiredNativeTimerSpecification.day = "2026-09-23";
    value.desiredNativeTimerSpecification.weekdays = "-------";
    value.desiredNativeTimerSpecification.startTime = "1000";
    value.desiredNativeTimerSpecification.endTime = "1100";
    value.desiredNativeTimerSpecification.priority = 50;
    value.desiredNativeTimerSpecification.lifetime = 99;
    value.desiredNativeTimerSpecification.enabled = true;
    value.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    return value;
}

TimerIntentApplicationRequest request()
{
    TimerIntentApplicationRequest value;
    value.intent = intent();
    value.timerAssignmentId = "assignment:application:1";
    value.candidates = {candidate()};
    value.activatedAt = 101;
    value.scheduledAt = 200;
    value.provisioningAt = 201;
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    TimerIntentRepository intents(database);
    TimerAssignmentRepository assignments(database);
    NativeTimerBindingRepository bindings(database);
    assert(intents.ensureSchema());
    assert(assignments.ensureSchema());
    assert(bindings.ensureSchema());

    TimerIntentApplicationService service(intents, assignments, bindings);
    // An accidental "in current transaction" call without the coordinator's
    // BEGIN may never leave a draft Intent behind in autocommit mode.
    auto forbiddenNested = request();
    forbiddenNested.intent.timerIntentId = "intent:requires-transaction";
    forbiddenNested.timerAssignmentId = "assignment:requires-transaction";
    assert(!database.transactionActive());
    assert(service.submitAndSelectPrimaryInCurrentTransaction(
        forbiddenNested).status == TimerIntentApplicationStatus::repositoryError);
    assert(intents.findById("intent:requires-transaction").status ==
        TimerIntentRepositoryStatus::notFound);
    assert(assignments.findById("assignment:requires-transaction").status ==
        TimerAssignmentRepositoryStatus::notFound);

    const auto first = service.submitAndProvisionPrimary(request());
    assert(first.status == TimerIntentApplicationStatus::provisioningStarted);
    assert(first.intent.state == TimerIntentState::active);
    assert(first.intent.intentRevision == "2");
    assert(first.assignment.state == TimerAssignmentState::provisioning);
    assert(first.assignment.role == TimerAssignmentRole::primary);
    assert(first.assignment.backendId == "backend:alpha");
    assert(first.assignment.backendGeneration == 7);
    assert(first.assignment.desiredNativeTimerSpecificationPresent);
    assert(first.assignment.desiredNativeTimerSpecification.title
        == "Application");

    // Stable identifiers allow restart-safe replay across every durable step.
    TimerIntentApplicationService restarted(intents, assignments, bindings);
    const auto replay = restarted.submitAndProvisionPrimary(request());
    assert(replay.status == TimerIntentApplicationStatus::alreadyProvisioning);
    assert(replay.assignment.timerAssignmentId ==
        first.assignment.timerAssignmentId);
    assert(replay.assignment.desiredNativeTimerSpecificationPresent);
    assert(replay.assignment.desiredNativeTimerSpecification.channelId
        == first.assignment.desiredNativeTimerSpecification.channelId);

    auto changed = request();
    changed.intent.spec.schedule.stopAt = 2100;
    assert(restarted.submitAndProvisionPrimary(changed).status ==
        TimerIntentApplicationStatus::intentConflict);

    auto unavailable = request();
    unavailable.intent.timerIntentId = "intent:application:2";
    unavailable.timerAssignmentId = "assignment:application:2";
    unavailable.candidates.front().state =
        TimerAssignmentPlanningBackendState::offline;
    const auto noBackend = restarted.submitAndProvisionPrimary(unavailable);
    assert(noBackend.status == TimerIntentApplicationStatus::noEligibleBackend);
    assert(noBackend.assignment.state == TimerAssignmentState::unassigned);


    // Public-v1 preparation must not consume the selected Assignment:
    // NativeTimerCreateAdmissionService performs that change atomically
    // together with persistent Operation preparation.
    auto selectedRequest = request();
    selectedRequest.intent.timerIntentId = "intent:application:selection";
    selectedRequest.timerAssignmentId = "assignment:application:selection";
    const auto selected = service.submitAndSelectPrimary(selectedRequest);
    assert(selected.ok());
    assert(selected.status == TimerIntentApplicationStatus::assignmentSelected);
    assert(selected.intent.state == TimerIntentState::active);
    assert(selected.assignment.state == TimerAssignmentState::selected);
    assert(selected.assignment.assignmentRevision == "1");
    assert(selected.assignment.desiredNativeTimerSpecificationPresent);
    const auto durable = assignments.findById(selectedRequest.timerAssignmentId);
    assert(durable.ok());
    assert(durable.assignment.state == TimerAssignmentState::selected);
    assert(durable.assignment.assignmentRevision ==
        selected.assignment.assignmentRevision);

    TimerIntentApplicationService selectAfterRestart(
        intents, assignments, bindings);
    const auto selectedReplay =
        selectAfterRestart.submitAndSelectPrimary(selectedRequest);
    assert(selectedReplay.status ==
        TimerIntentApplicationStatus::assignmentSelected);
    assert(selectedReplay.assignment.timerAssignmentId ==
        selected.assignment.timerAssignmentId);
    assert(selectedReplay.assignment.assignmentRevision ==
        selected.assignment.assignmentRevision);

    const auto provisioning =
        selectAfterRestart.submitAndProvisionPrimary(selectedRequest);
    assert(provisioning.status ==
        TimerIntentApplicationStatus::provisioningStarted);
    assert(provisioning.assignment.state ==
        TimerAssignmentState::provisioning);
    const auto afterAdmission =
        selectAfterRestart.submitAndSelectPrimary(selectedRequest);
    assert(afterAdmission.status ==
        TimerIntentApplicationStatus::alreadyProvisioning);
    assert(afterAdmission.assignment.state ==
        TimerAssignmentState::provisioning);

    auto wrongRequest = selectedRequest;
    wrongRequest.intent.spec.schedule.stopAt = 2150;
    assert(selectAfterRestart.submitAndSelectPrimary(wrongRequest).status ==
        TimerIntentApplicationStatus::intentConflict);


    // F5.5.5: the manual-window input follows the already implemented
    // TimerIntent -> selected Assignment repository path. Native CREATE
    // remains owned by NativeTimerCreateAdmissionService (not this test).
    auto manualRequest = request();
    manualRequest.intent.timerIntentId = "intent:application:manual";
    manualRequest.timerAssignmentId = "assignment:application:manual";
    manualRequest.intent.createdByActorId = "actor:manual";
    auto native = manualRequest.candidates.front().desiredNativeTimerSpecification;
    native.title = "Manual Archive, Part 1";
    native.directory = "TV~Archiv";
    native.day = "2026-11-24";
    native.startTime = "2015";
    native.endTime = "2130";
    native.priority = 0;
    native.lifetime = 0;
    native.vps = true;

    ManualTimerSourceEvidence source;
    source.backendId = "backend:alpha";
    source.sourceType = "dvb";
    source.sourceId = "suitebridge:local";
    source.backendChannelId = native.channelId;
    source.timezone = "Europe/Berlin";
    source.startAt = 1795547700;
    source.stopAt = 1795552200;
    const auto manualDraft =
        makeManualWindowTimerIntentDraft(native, source, "actor:manual");
    assert(manualDraft.valid);
    manualRequest.intent.spec = manualDraft.intent;
    manualRequest.intent.expiresAt = source.stopAt + 86400;
    manualRequest.candidates.front().desiredNativeTimerSpecification =
        manualDraft.desiredNativeTimer;
    manualRequest.candidates.front().channel.sourceType = source.sourceType;
    manualRequest.candidates.front().channel.sourceId = source.sourceId;
    manualRequest.candidates.front().channel.sourceChannelId =
        source.backendChannelId;

    const auto manualSelected =
        service.submitAndSelectPrimary(manualRequest);
    assert(manualSelected.status == TimerIntentApplicationStatus::assignmentSelected);
    assert(manualSelected.assignment.state == TimerAssignmentState::selected);
    assert(manualSelected.assignment.desiredNativeTimerSpecificationPresent);
    assert(manualSelected.assignment.desiredNativeTimerSpecification.title ==
        native.title);
    assert(manualSelected.assignment.desiredNativeTimerSpecification.directory ==
        native.directory);
    assert(manualSelected.assignment.desiredNativeTimerSpecification.priority == 0);
    assert(manualSelected.assignment.desiredNativeTimerSpecification.lifetime == 0);
    assert(manualSelected.assignment.desiredNativeTimerSpecification.vps);
    assert(manualSelected.intent.spec.intentType == TimerIntentType::manualWindow);
    assert(manualSelected.intent.spec.programEventId.empty());

    const auto manualPersisted =
        assignments.findById(manualRequest.timerAssignmentId);
    assert(manualPersisted.ok());
    assert(manualPersisted.assignment.state == TimerAssignmentState::selected);
    assert(manualPersisted.assignment.desiredNativeTimerSpecification.title ==
        native.title);
    assert(manualPersisted.assignment.desiredNativeTimerSpecification.directory ==
        native.directory);
    assert(manualPersisted.assignment.desiredNativeTimerSpecification.priority == 0);
    assert(manualPersisted.assignment.desiredNativeTimerSpecification.lifetime == 0);
    const auto manualReplay = restarted.submitAndSelectPrimary(manualRequest);
    assert(manualReplay.status == TimerIntentApplicationStatus::assignmentSelected);
    assert(manualReplay.assignment.assignmentRevision ==
        manualPersisted.assignment.assignmentRevision);

    std::cout << "test_timer_intent_application_service passed\n";
    return 0;
}
