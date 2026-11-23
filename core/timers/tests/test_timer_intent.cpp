#include "TimerIntent.h"
#include "ManualTimerIntentDraft.h"

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

    // F5.5.5: map an explicitly entered native one-shot timer to the
    // existing manualWindow + precise desired Assignment specification.
    // Backend-local clock/source evidence is supplied by the server, never
    // derived by parsing local time on the daemon.
    NativeTimerSpecification native;
    native.channelId = "S19.2E-1-1019-10301";
    native.title = "News, Spezial";
    native.directory = "Nachrichten~Archiv";
    native.day = "2026-11-24";
    native.weekdays = "-------";
    native.startTime = "2015";
    native.endTime = "2030";
    native.priority = 0;
    native.lifetime = 0;
    native.enabled = false;
    native.vps = true;

    ManualTimerSourceEvidence source;
    source.backendId = "default";
    source.sourceType = "dvb";
    source.sourceId = "suitebridge:local";
    source.backendChannelId = native.channelId;
    source.timezone = "Europe/Berlin";
    source.startAt = 1795547700;
    source.stopAt = 1795548600;

    const auto draft = makeManualWindowTimerIntentDraft(native, source, "actor:1");
    assert(draft.valid);
    assert(timerIntentValidSpec(draft.intent));
    assert(draft.intent.intentType == TimerIntentType::manualWindow);
    assert(draft.intent.programEventId.empty());
    assert(draft.intent.backendEventRef.eventId.empty());
    assert(draft.intent.ownerActorId == "actor:1");
    assert(draft.intent.channelRequirement.sourceType == "dvb");
    assert(draft.intent.channelRequirement.sourceId == "suitebridge:local");
    assert(draft.intent.channelRequirement.sourceChannelId == native.channelId);
    assert(draft.intent.schedule.timezone == "Europe/Berlin");
    assert(draft.intent.schedule.startAt == source.startAt);
    assert(draft.intent.schedule.stopAt == source.stopAt);
    assert(draft.intent.recordingOptions.priority == 0);
    assert(draft.intent.recordingOptions.lifetimeDays == 0);
    assert(draft.intent.recordingOptions.vpsPreferred);
    assert(draft.intent.assignmentPolicy.preferredBackendIds.size() == 1);
    assert(draft.intent.assignmentPolicy.preferredBackendIds[0] == "default");
    assert(draft.desiredNativeTimer.title == native.title);
    assert(draft.desiredNativeTimer.directory == native.directory);
    assert(draft.desiredNativeTimer.day == native.day);
    assert(draft.desiredNativeTimer.startTime == native.startTime);
    assert(draft.desiredNativeTimer.endTime == native.endTime);
    assert(draft.desiredNativeTimer.priority == 0);
    assert(draft.desiredNativeTimer.lifetime == 0);
    assert(!draft.desiredNativeTimer.enabled);
    assert(draft.desiredNativeTimer.vps);
    assert(draft.desiredNativeTimer.weekdays == "-------");

    auto wrongSource = source;
    wrongSource.backendChannelId = "S19.2E-1-1019-99999";
    assert(!makeManualWindowTimerIntentDraft(native, wrongSource, "actor:1").valid);
    wrongSource = source;
    wrongSource.stopAt = wrongSource.startAt;
    assert(!makeManualWindowTimerIntentDraft(native, wrongSource, "actor:1").valid);
    wrongSource = source;
    wrongSource.timezone.clear();
    assert(!makeManualWindowTimerIntentDraft(native, wrongSource, "actor:1").valid);
    auto recurringNative = native;
    recurringNative.weekdays = "MTWTFSS";
    assert(!makeManualWindowTimerIntentDraft(recurringNative, source, "actor:1").valid);
    auto missingDay = native;
    missingDay.day.clear();
    assert(!makeManualWindowTimerIntentDraft(missingDay, source, "actor:1").valid);
    assert(!makeManualWindowTimerIntentDraft(native, source, "").valid);

    // Recurring timers reuse the existing intent/assignment model, while
    // remaining rejected by the one-shot-only projection.
    auto repeated = native;
    repeated.weekdays = "MTWTF--";
    const auto weekly =
        makeRecurringTimerIntentDraft(repeated, source, "actor:1");
    assert(weekly.valid);
    assert(timerIntentValidSpec(weekly.intent));
    assert(weekly.intent.intentType == TimerIntentType::recurringSchedule);
    assert(weekly.intent.schedule.recurrenceRule ==
        "FREQ=WEEKLY;BYDAY=MO,TU,WE,TH,FR");
    assert(weekly.intent.schedule.timezone == "Europe/Berlin");
    assert(weekly.intent.schedule.startAt == source.startAt);
    assert(weekly.intent.schedule.stopAt == source.stopAt);
    assert(weekly.intent.channelRequirement.sourceChannelId == native.channelId);
    assert(weekly.intent.recordingOptions.priority == 0);
    assert(weekly.intent.recordingOptions.lifetimeDays == 0);
    assert(weekly.intent.recordingOptions.vpsPreferred);
    assert(weekly.desiredNativeTimer.weekdays == "MTWTF--");
    assert(weekly.desiredNativeTimer.day == native.day);
    assert(!weekly.desiredNativeTimer.enabled);
    assert(weekly.desiredNativeTimer.vps);
    assert(timerIntentSemanticIdentity(weekly.intent) !=
        timerIntentSemanticIdentity(draft.intent));

    repeated.weekdays = "------S";
    const auto sunday =
        makeRecurringTimerIntentDraft(repeated, source, "actor:1");
    assert(sunday.valid);
    assert(sunday.intent.schedule.recurrenceRule == "FREQ=WEEKLY;BYDAY=SU");
    repeated.weekdays = "-T----S";
    const auto tuesdaySunday =
        makeRecurringTimerIntentDraft(repeated, source, "actor:1");
    assert(tuesdaySunday.valid);
    assert(tuesdaySunday.intent.schedule.recurrenceRule ==
        "FREQ=WEEKLY;BYDAY=TU,SU");

    // Positional ambiguity (both T and S occur twice) is never guessed.
    repeated.weekdays = "T------";
    assert(!makeRecurringTimerIntentDraft(repeated, source, "actor:1").valid);
    repeated.weekdays = "------M";
    assert(!makeRecurringTimerIntentDraft(repeated, source, "actor:1").valid);
    repeated.weekdays = "-------";
    assert(!makeRecurringTimerIntentDraft(repeated, source, "actor:1").valid);
    repeated.weekdays = "MTWTFSS";
    auto badSource = source;
    badSource.backendChannelId = "different-channel";
    assert(!makeRecurringTimerIntentDraft(repeated, badSource, "actor:1").valid);
    badSource = source;
    badSource.timezone.clear();
    assert(!makeRecurringTimerIntentDraft(repeated, badSource, "actor:1").valid);
    badSource = source;
    badSource.stopAt = badSource.startAt;
    assert(!makeRecurringTimerIntentDraft(repeated, badSource, "actor:1").valid);
    badSource = source;
    badSource.stopAt += 24 * 3600;
    assert(!makeRecurringTimerIntentDraft(repeated, badSource, "actor:1").valid);
    assert(!makeRecurringTimerIntentDraft(repeated, source, "").valid);

    return 0;
}
