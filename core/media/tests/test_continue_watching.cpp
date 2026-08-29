#include "ContinueWatching.h"
#include "Database.h"

#include <cassert>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace
{

ContinueWatchingRecordingTruth recording(
    const std::string& backendId,
    const std::string& recordingId,
    int durationSeconds,
    bool durationKnown = true)
{
    ContinueWatchingRecordingTruth value;
    value.backendId = backendId;
    value.recordingId = recordingId;
    value.title = "Recording " + recordingId;
    value.subtitle = "Episode";
    value.durationSeconds = durationSeconds;
    value.durationKnown = durationKnown;
    return value;
}

} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    ContinueWatchingRepository repository(database);
    assert(repository.ensureSchema());

    std::unordered_map<std::string, ContinueWatchingRecordingTruth> recordings;
    auto put = [&](const ContinueWatchingRecordingTruth& value) {
        recordings[value.backendId + ":" + value.recordingId] = value;
    };
    auto resolve = [&](const std::string& backendId, const std::string& recordingId)
        -> std::optional<ContinueWatchingRecordingTruth> {
        const auto it = recordings.find(backendId + ":" + recordingId);
        if (it == recordings.end()) return std::nullopt;
        return it->second;
    };

    put(recording("default", "r1", 1800));
    put(recording("default", "r2", 0, false));
    put(recording("other", "r1", 1800));

    ContinueWatchingService service(repository, resolve);

    // Position zero and playback without canonical resume support never become Continue Watching truth.
    assert(service.recordProgress("actor-a", "default", "r1", 0, true, "op-zero"));
    assert(service.list("actor-a", "default").empty());
    assert(service.recordProgress("actor-a", "default", "r1", 120, false, "op-no-resume"));
    assert(service.list("actor-a", "default").empty());

    // A real unfinished canonical absolute position is persisted and projected.
    assert(service.recordProgress("actor-a", "default", "r1", 120, true, "op-1"));
    auto items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r1");
    assert(items[0].resumePositionSeconds == 120);
    assert(items[0].recording.durationKnown);
    assert(items[0].recording.durationSeconds == 1800);

    // Actor and backend scopes are isolated.
    assert(service.list("actor-b", "default").empty());
    assert(service.list("actor-a", "other").empty());
    assert(service.recordProgress("actor-a", "other", "r1", 77, true, "op-other"));
    assert(service.list("actor-a", "other").size() == 1);
    assert(service.list("actor-a", "default")[0].resumePositionSeconds == 120);

    // Unknown duration is allowed as truthful resumable state; callers must not invent a percentage.
    assert(service.recordProgress("actor-a", "default", "r2", 33, true, "op-unknown"));
    items = service.list("actor-a", "default");
    bool sawUnknownDuration = false;
    for (const auto& item : items) {
        if (item.recording.recordingId == "r2") {
            sawUnknownDuration = true;
            assert(!item.recording.durationKnown);
            assert(item.recording.durationSeconds == 0);
            assert(item.resumePositionSeconds == 33);
        }
    }
    assert(sawUnknownDuration);

    // Completion is exact: known position >= known duration removes the item; no invented near-end threshold.
    assert(service.recordProgress("actor-a", "default", "r1", 1799, true, "op-near-end"));
    items = service.list("actor-a", "default");
    bool sawNearEnd = false;
    for (const auto& item : items) {
        if (item.recording.recordingId == "r1") {
            sawNearEnd = true;
            assert(item.resumePositionSeconds == 1799);
        }
    }
    assert(sawNearEnd);
    assert(service.recordProgress("actor-a", "default", "r1", 1800, true, "op-complete"));
    items = service.list("actor-a", "default");
    for (const auto& item : items) assert(item.recording.recordingId != "r1");

    // Duplicate operation replay is idempotent and never creates a history row.
    assert(service.recordProgress("actor-a", "default", "r2", 40, true, "op-idempotent"));
    assert(service.recordProgress("actor-a", "default", "r2", 90, true, "op-idempotent"));
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r2");
    assert(items[0].resumePositionSeconds == 40);

    // A later accepted operation may move backwards after a legitimate user seek.
    assert(service.recordProgress("actor-a", "default", "r2", 20, true, "op-backward"));
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].resumePositionSeconds == 20);

    // Actual playback activity orders the rail, not title/recording creation time.
    put(recording("default", "r3", 900));
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    assert(service.recordProgress("actor-a", "default", "r3", 15, true, "op-newer"));
    items = service.list("actor-a", "default");
    assert(items.size() == 2);
    assert(items[0].recording.recordingId == "r3");

    // Deleted/stale current recording identity fails closed and is cleaned from durable current-state rows.
    recordings.erase("default:r3");
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r2");
    assert(repository.findForActorBackend("actor-a", "default").size() == 1);

    // Explicit clear removes only the scoped current state.
    assert(service.clear("actor-a", "default", "r2", "op-clear"));
    assert(service.list("actor-a", "default").empty());
    assert(service.list("actor-a", "other").size() == 1);

    return 0;
}
