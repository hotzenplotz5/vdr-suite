#include "ContinueWatching.h"

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

ContinueWatchingService::ContinueWatchingService(
    ContinueWatchingRepository& repository,
    RecordingIdResolver idResolver,
    RecordingNativeResolver nativeResolver)
    : repository_(repository),
      idResolver_(std::move(idResolver)),
      nativeResolver_(std::move(nativeResolver))
{
}

bool ContinueWatchingService::recordProgress(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    int positionSeconds,
    bool resumeSupported,
    const std::string& operationId)
{
    if (!validScope(actorId, backendId, recordingId) || operationId.empty())
        return false;

    const auto current =
        idResolver_ ? idResolver_(backendId, recordingId) : std::nullopt;
    if (!current.has_value() ||
        !resumeSupported ||
        positionSeconds <= 0 ||
        (current->durationKnown && current->durationSeconds > 0 &&
         positionSeconds >= current->durationSeconds))
    {
        return repository_.clear(actorId, backendId, recordingId);
    }

    if (current->backendNativeId.empty())
        return repository_.clear(actorId, backendId, recordingId);

    return repository_.upsert(
        actorId,
        backendId,
        recordingId,
        current->backendNativeId,
        positionSeconds,
        operationId);
}

bool ContinueWatchingService::clear(
    const std::string& actorId,
    const std::string& backendId,
    const std::string& recordingId,
    const std::string&)
{
    const auto current =
        idResolver_ ? idResolver_(backendId, recordingId) : std::nullopt;
    return repository_.clear(
        actorId,
        backendId,
        recordingId,
        current.has_value() ? current->backendNativeId : std::string());
}

std::vector<ContinueWatchingItem> ContinueWatchingService::list(
    const std::string& actorId,
    const std::string& backendId)
{
    std::vector<ContinueWatchingItem> items;
    const auto states = repository_.findForActorBackend(actorId, backendId);
    items.reserve(states.size());

    for (const auto& state : states) {
        if (state.backendNativeId.empty()) {
            repository_.clear(state.actorId, state.backendId, state.recordingId);
            continue;
        }

        const auto current = nativeResolver_
            ? nativeResolver_(state.backendId, state.backendNativeId)
            : std::nullopt;
        const bool invalid =
            !current.has_value() ||
            state.positionSeconds <= 0 ||
            (current->durationKnown && current->durationSeconds > 0 &&
             state.positionSeconds >= current->durationSeconds);
        if (invalid) {
            repository_.clear(
                state.actorId,
                state.backendId,
                state.recordingId,
                state.backendNativeId);
            continue;
        }

        ContinueWatchingItem item;
        item.recording = *current;
        item.resumePositionSeconds = state.positionSeconds;
        item.lastActivityAt = state.lastActivityAt;
        items.push_back(std::move(item));
    }
    return items;
}
