#include "TimerAssignmentPlanner.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

using namespace vdrsuite::timers;

namespace
{
TimerIntent makeSourceIntent()
{
    TimerIntent intent;
    intent.timerIntentId = "timer-intent:source-channel";
    intent.intentRevision = "5";
    intent.state = TimerIntentState::active;
    intent.createdByActorId = "actor:owner";
    intent.spec.intentType = TimerIntentType::manualWindow;
    intent.spec.ownerActorId = "actor:owner";
    intent.spec.channelRequirement.sourceType = "guide";
    intent.spec.channelRequirement.sourceId = "guide:one";
    intent.spec.channelRequirement.sourceChannelId = "guide-channel:news";
    intent.spec.schedule.startAt = 1000;
    intent.spec.schedule.stopAt = 2000;
    intent.spec.schedule.timezone = "Europe/Rome";
    intent.createdAt = 100;
    intent.updatedAt = 100;
    return intent;
}

TimerAssignmentPlanningBackendCandidate makeCandidate()
{
    TimerAssignmentPlanningBackendCandidate candidate;
    candidate.backendId = "backend:a";
    candidate.siteId = "site:one";
    candidate.currentBackendGeneration = 4;
    candidate.state = TimerAssignmentPlanningBackendState::online;
    candidate.writeAllowed = true;
    candidate.executionAuthorityCurrent = true;
    candidate.executionAuthorityFence = "authority:vdr-native:4";
    candidate.capability.backendGeneration = 4;
    candidate.capability.revision = "cap:9";
    candidate.capability.current = true;
    candidate.capability.timerCreate = true;
    candidate.capability.timerReadback = true;
    candidate.health.backendGeneration = 4;
    candidate.health.revision = "health:11";
    candidate.health.current = true;
    candidate.health.state = TimerAssignmentPlanningHealthState::healthy;
    candidate.health.timerWritesAvailable = true;
    candidate.channel.backendGeneration = 4;
    candidate.channel.mappingRevision = "channel-map:5";
    candidate.channel.mappingSource = "guide-mapping";
    candidate.channel.sourceType = "guide";
    candidate.channel.sourceId = "guide:one";
    candidate.channel.sourceChannelId = "guide-channel:news";
    candidate.channel.backendChannelId = "S19.2E-1-1011-11100";
    candidate.channel.current = true;
    candidate.conflict = TimerAssignmentPlanningConflictState::confirmedClear;
    return candidate;
}

bool containsFragment(
    const std::vector<std::string>& values,
    const std::string& fragment)
{
    return std::any_of(
        values.begin(),
        values.end(),
        [&](const std::string& value)
        {
            return value.find(fragment) != std::string::npos;
        });
}
}

int main()
{
    TimerAssignmentPlanningRequest request;
    request.intent = makeSourceIntent();
    request.candidates = {makeCandidate()};

    const auto selected = planTimerAssignment(request);
    assert(selected.outcome == TimerAssignmentPlanningOutcome::selected);
    assert(selected.selectedBackendId == "backend:a");

    request.candidates[0].channel.sourceChannelId = "guide-channel:sport";
    const auto mismatch = planTimerAssignment(request);
    assert(mismatch.outcome == TimerAssignmentPlanningOutcome::unassigned);
    assert(containsFragment(
        mismatch.decisionEvidence.exclusions,
        "channel_source_mismatch"));

    request.candidates[0] = makeCandidate();
    request.candidates[0].channel.current = false;
    const auto stale = planTimerAssignment(request);
    assert(stale.outcome == TimerAssignmentPlanningOutcome::unassigned);
    assert(containsFragment(
        stale.decisionEvidence.exclusions,
        "channel_mapping_missing_or_stale"));

    return 0;
}
