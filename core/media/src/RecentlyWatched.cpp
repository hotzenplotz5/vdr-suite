#include "RecentlyWatched.h"

#include <utility>

namespace
{

bool validScope(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId)
{
    return !actorId.empty() && !backendId.empty() && !recordingId.empty();
}

} // namespace

RecentlyWatchedService::RecentlyWatchedService(
    RecentlyWatchedRepository& repository,
    RecordingResolver resolver)
    : repository_(repository),
      resolver_(std::move(resolver))
{
}

bool RecentlyWatchedService::recordActivity(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    int positionSeconds,
    bool positionKnown,
    bool resumeSupportKnown,
    bool resumeSupported,
    bool ended,
    const std::string& operationId)
{
    if (!validScope(actorId, backendId, recordingId) || operationId.empty() ||
        positionSeconds < 0)
    {
        return false;
    }

    const auto current = resolver_ ? resolver_(backendId, recordingId) : std::nullopt;
    if (!current.has_value()) return false;

    RecentlyWatchedState state;
    state.actorId = actorId;
    state.backendId = backendId;
    state.recordingId = recordingId;
    state.positionSeconds = positionKnown ? positionSeconds : 0;
    state.positionKnown = positionKnown;
    state.sourceEvidence = CanonicalPlaybackEvidence;
    state.lastOperationId = operationId;

    if (ended) {
        state.completionKnown = true;
        state.completed = true;
    } else if (positionKnown && current->durationKnown && current->durationSeconds > 0) {
        state.completionKnown = true;
        state.completed = positionSeconds >= current->durationSeconds;
    }

    if (state.completionKnown && state.completed) {
        state.resumeRelevanceKnown = true;
        state.resumeRelevant = false;
    } else if (resumeSupportKnown) {
        state.resumeRelevanceKnown = true;
        state.resumeRelevant = resumeSupported && positionKnown && positionSeconds > 0;
    }

    return repository_.record(state);
}

std::vector<RecentlyWatchedItem> RecentlyWatchedService::list(
    const std::string& actorId,
    const std::string& backendId)
{
    std::vector<RecentlyWatchedItem> items;
    const auto states = repository_.findForActorBackend(actorId, backendId);
    items.reserve(states.size());

    for (const auto& state : states) {
        const auto current = resolver_ ? resolver_(state.backendId, state.recordingId) : std::nullopt;
        if (!current.has_value()) {
            repository_.remove(state.actorId, state.backendId, state.recordingId);
            continue;
        }

        RecentlyWatchedItem item;
        item.recording = *current;
        item.positionSeconds = state.positionSeconds;
        item.positionKnown = state.positionKnown;
        item.completionKnown = state.completionKnown;
        item.completed = state.completed;
        item.resumeRelevanceKnown = state.resumeRelevanceKnown;
        item.resumeRelevant = state.resumeRelevant;
        item.sourceEvidence = state.sourceEvidence;
        item.lastActivityAt = state.lastActivityAt;
        items.push_back(std::move(item));
    }

    return items;
}
