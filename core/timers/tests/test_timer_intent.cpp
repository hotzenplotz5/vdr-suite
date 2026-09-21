#include "TimerIntent.h"

#include <cassert>
#include <string>

using namespace vdrsuite::timers;

namespace
{
TimerIntentSpec validProgrammeEvent()
{
    TimerIntentSpec spec;
    spec.intentType = TimerIntentType::programmeEvent;
    spec.ownerActorId = "actor:user:1";
    spec.automationSource = {"searchtimer", "search:crime", "occurrence:42"};
    spec.programEventId = "programme:event:1";
    spec.channelRequirement.canonicalChannelId = "channel:ard-hd";
    spec.schedule = {1786385700, 1786392900, "Europe/Rome", ""};
    spec.recordingOptions.startMarginSeconds = 120;
    spec.recordingOptions.stopMarginSeconds = 300;
    spec.recordingOptions.directoryPolicy = "series";
    spec.recordingOptions.namingPolicy = "event-title";
    spec.recordingOptions.retentionPolicyReference = "retention:default";
    spec.assignmentPolicy.allowFailover = true;
    spec.assignmentPolicy.preferredBackendIds = {"backend:living-room"};
    spec.assignmentPolicy.excludedBackendIds = {"backend:maintenance"};
    return spec;
}

TimerIntent validIntent()
{
    TimerIntent intent;
    intent.timerIntentId = "timer-intent:1";
    intent.intentRevision = "intent-revision:7";
    intent.state = TimerIntentState::active;
    intent.createdByActorId = "actor:user:1";
    intent.spec = validProgrammeEvent();
    intent.createdAt = 1786200000;
    intent.updatedAt = 1786200300;
    intent.expiresAt = 1786396500;
    return intent;
}
}

int main()
{
    const auto base = validProgrammeEvent();
    assert(timerIntentValidSpec(base));
    assert(std::string(timerIntentTypeName(base.intentType)) == "programme_event");
    assert(!timerIntentSemanticIdentity(base).empty());

    auto fallbackEvent = base;
    fallbackEvent.programEventId.clear();
    fallbackEvent.backendEventRef = {"backend:a", "S19.2E-1-1019-10301", "987654", "vdr-epg"};
    assert(timerIntentValidSpec(fallbackEvent));
    fallbackEvent.backendEventRef.sourceId.clear();
    assert(!timerIntentValidSpec(fallbackEvent));

    auto manual = base;
    manual.intentType = TimerIntentType::manualWindow;
    manual.programEventId.clear();
    manual.automationSource = {};
    assert(timerIntentValidSpec(manual));
    manual.programEventId = "programme:event:1";
    assert(!timerIntentValidSpec(manual));
    manual.programEventId.clear();
    manual.schedule.recurrenceRule = "FREQ=WEEKLY;BYDAY=MO";
    assert(!timerIntentValidSpec(manual));

    auto recurring = base;
    recurring.intentType = TimerIntentType::recurringSchedule;
    recurring.programEventId.clear();
    recurring.automationSource = {};
    recurring.schedule.recurrenceRule = "FREQ=WEEKLY;BYDAY=MO,WE";
    assert(timerIntentValidSpec(recurring));
    recurring.schedule.recurrenceRule.clear();
    assert(!timerIntentValidSpec(recurring));

    auto sourceChannel = base;
    sourceChannel.channelRequirement = {"", "dvb", "site:home", "S19.2E-1-1019-10301"};
    assert(timerIntentValidSpec(sourceChannel));
    sourceChannel.channelRequirement.sourceId.clear();
    assert(!timerIntentValidSpec(sourceChannel));

    auto policy = base;
    policy.assignmentPolicy.preferredBackendIds.push_back("backend:maintenance");
    assert(!timerIntentValidSpec(policy));
    policy = base;
    policy.assignmentPolicy.preferredBackendIds.push_back("backend:living-room");
    assert(!timerIntentValidSpec(policy));

    auto replicas = base;
    replicas.replicaPolicy.desiredAssignments = 0;
    assert(!timerIntentValidSpec(replicas));
    replicas.replicaPolicy.desiredAssignments = 5;
    assert(!timerIntentValidSpec(replicas));
    replicas.replicaPolicy.desiredAssignments = 2;
    replicas.replicaPolicy.requireBackendDiversity = true;
    replicas.replicaPolicy.requireSiteDiversity = true;
    assert(!timerIntentValidSpec(replicas));
    replicas.replicaPolicy.simultaneousRecordingIntentional = true;
    replicas.replicaPolicy.rationale = "deliberate disaster-recovery replica";
    replicas.replicaPolicy.storagePolicyReference = "storage:separate-site";
    replicas.replicaPolicy.retentionPolicyReference = "retention:important";
    assert(timerIntentValidSpec(replicas));

    auto intent = validIntent();
    assert(timerIntentValid(intent));
    assert(timerIntentAssignable(intent.state));
    assert(!timerIntentTerminal(intent.state));
    assert(timerIntentRevisionMatches("intent-revision:7", intent.intentRevision));
    assert(!timerIntentRevisionMatches("intent-revision:6", intent.intentRevision));
    intent.updatedAt = intent.createdAt - 1;
    assert(!timerIntentValid(intent));

    assert(timerIntentCanTransition(TimerIntentState::draft, TimerIntentState::active));
    assert(timerIntentCanTransition(TimerIntentState::active, TimerIntentState::paused));
    assert(timerIntentCanTransition(TimerIntentState::paused, TimerIntentState::active));
    assert(timerIntentCanTransition(TimerIntentState::active, TimerIntentState::cancelRequested));
    assert(timerIntentCanTransition(TimerIntentState::cancelRequested, TimerIntentState::cancelled));
    assert(!timerIntentCanTransition(TimerIntentState::active, TimerIntentState::cancelled));
    assert(!timerIntentCanTransition(TimerIntentState::satisfied, TimerIntentState::active));
    assert(timerIntentTerminal(TimerIntentState::satisfied));
    assert(timerIntentTerminal(TimerIntentState::cancelled));

    auto collisionLeft = base;
    auto collisionRight = base;
    collisionLeft.ownerActorId = "actor:1|2";
    collisionRight.ownerActorId = "actor:1";
    collisionRight.automationSource.sourceType = "2|searchtimer";
    assert(timerIntentSemanticIdentity(collisionLeft) != timerIntentSemanticIdentity(collisionRight));

    const auto identity = timerIntentSemanticIdentity(base);
    auto changed = base;
    changed.schedule.stopAt += 60;
    assert(timerIntentSemanticIdentity(changed) != identity);
    changed = base; changed.recordingOptions.vpsPreferred = true;
    assert(timerIntentSemanticIdentity(changed) != identity);
    changed = base; changed.assignmentPolicy.allowFailover = false;
    assert(timerIntentSemanticIdentity(changed) != identity);
    changed = base; changed.replicaPolicy.desiredAssignments = 2; changed.replicaPolicy.simultaneousRecordingIntentional = true; changed.replicaPolicy.rationale = "explicit replica";
    assert(timerIntentSemanticIdentity(changed) != identity);
    changed = base; changed.duplicatePolicy.requireOperatorReviewOnAmbiguity = false;
    assert(timerIntentSemanticIdentity(changed) != identity);

    return 0;
}
