#include "Database.h"
#include "NativeTimerBindingRepository.h"
#include "TimerAssignmentRepository.h"
#include "TimerIntentApplicationService.h"
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
    const auto first = service.submitAndProvisionPrimary(request());
    assert(first.status == TimerIntentApplicationStatus::provisioningStarted);
    assert(first.intent.state == TimerIntentState::active);
    assert(first.intent.intentRevision == "2");
    assert(first.assignment.state == TimerAssignmentState::provisioning);
    assert(first.assignment.role == TimerAssignmentRole::primary);
    assert(first.assignment.backendId == "backend:alpha");
    assert(first.assignment.backendGeneration == 7);

    // Stable identifiers allow restart-safe replay across every durable step.
    TimerIntentApplicationService restarted(intents, assignments, bindings);
    const auto replay = restarted.submitAndProvisionPrimary(request());
    assert(replay.status == TimerIntentApplicationStatus::alreadyProvisioning);
    assert(replay.assignment.timerAssignmentId ==
        first.assignment.timerAssignmentId);

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

    std::cout << "test_timer_intent_application_service passed\n";
    return 0;
}
