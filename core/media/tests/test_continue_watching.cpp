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
    value.backendNativeId = "/recordings/" + recordingId + ".rec";
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

    auto resolveNative = [&](const std::string& backendId, const std::string& backendNativeId)
        -> std::optional<ContinueWatchingRecordingTruth> {
        for (const auto& entry : recordings) {
            if (entry.second.backendId == backendId &&
                entry.second.backendNativeId == backendNativeId)
                return entry.second;
        }
        return std::nullopt;
    };

    ContinueWatchingService service(repository, resolve, resolveNative);

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

    // A changed backend list number must still resolve the same Recording
    // through its persisted native identity.
    auto renumbered = recording("default", "r3-new", 900);
    renumbered.backendNativeId = "/recordings/r3.rec";
    recordings.erase("default:r3");
    put(renumbered);
    items = service.list("actor-a", "default");
    assert(items.size() == 2);
    assert(items[0].recording.recordingId == "r3-new");
    assert(items[0].resumePositionSeconds == 15);

    // Deleted/stale native identity fails closed and is cleaned.
    recordings.erase("default:r3-new");
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r2");
    assert(repository.findForActorBackend("actor-a", "default").size() == 1);

    // Explicit clear removes only the scoped current state.
    assert(service.clear("actor-a", "default", "r2", "op-clear"));
    assert(service.list("actor-a", "default").empty());
    assert(service.list("actor-a", "other").size() == 1);

    // Existing pre-migration rows have no native identity evidence. They must
    // be discarded instead of binding a reused numeric ID to today's Recording.
    Database legacyDatabase;
    assert(legacyDatabase.open(":memory:"));
    assert(legacyDatabase.execute(
        "CREATE TABLE continue_watching_state("
        "actor_id TEXT NOT NULL, backend_id TEXT NOT NULL, recording_id TEXT NOT NULL, "
        "position_seconds INTEGER NOT NULL, last_activity_at TEXT NOT NULL, "
        "last_operation_id TEXT NOT NULL, PRIMARY KEY(actor_id, backend_id, recording_id));"));
    assert(legacyDatabase.execute(
        "INSERT INTO continue_watching_state VALUES("
        "'legacy','default','r2',33,'2026-09-04T21:45:02.580Z','legacy-op');"));
    ContinueWatchingRepository legacyRepository(legacyDatabase);
    assert(legacyRepository.ensureSchema());
    ContinueWatchingService legacyService(legacyRepository, resolve, resolveNative);
    assert(legacyService.list("legacy", "default").empty());
    assert(legacyRepository.findForActorBackend("legacy", "default").empty());

    return 0;
}
