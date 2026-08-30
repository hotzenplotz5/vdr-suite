#include "RecentlyWatched.h"
#include "Database.h"

#include <cassert>
#include <chrono>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace
{
RecentlyWatchedRecordingTruth recording(
    const std::string& backendId,
    const std::string& recordingId,
    int durationSeconds,
    bool durationKnown = true)
{
    RecentlyWatchedRecordingTruth value;
    value.backendId = backendId;
    value.recordingId = recordingId;
    value.title = "Recording " + recordingId;
    value.durationSeconds = durationSeconds;
    value.durationKnown = durationKnown;
    return value;
}
} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));

    RecentlyWatchedRepository repository(database);
    assert(repository.ensureSchema());

    std::unordered_map<std::string, RecentlyWatchedRecordingTruth> recordings;
    auto put = [&](const RecentlyWatchedRecordingTruth& value) {
        recordings[value.backendId + ":" + value.recordingId] = value;
    };
    auto resolve = [&](const std::string& backendId, const std::string& recordingId)
        -> std::optional<RecentlyWatchedRecordingTruth> {
        const auto it = recordings.find(backendId + ":" + recordingId);
        return it == recordings.end() ? std::nullopt : std::optional<RecentlyWatchedRecordingTruth>(it->second);
    };

    put(recording("default", "r1", 100));
    put(recording("default", "r2", 0, false));
    put(recording("other", "r1", 100));
    RecentlyWatchedService service(repository, resolve);

    // Canonical active playback creates History independently of Continue Watching semantics.
    assert(service.recordActivity("actor-a", "default", "r1", 25, true, true, true, false, "op-1"));
    auto items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r1");
    assert(items[0].positionKnown && items[0].positionSeconds == 25);
    assert(items[0].completionKnown && !items[0].completed);
    assert(items[0].resumeRelevanceKnown && items[0].resumeRelevant);
    assert(items[0].sourceEvidence == RecentlyWatchedService::CanonicalPlaybackEvidence);

    // Completion remains in History and is explicitly non-resumable.
    assert(service.recordActivity("actor-a", "default", "r1", 100, true, true, true, true, "op-ended"));
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].completionKnown && items[0].completed);
    assert(items[0].resumeRelevanceKnown && !items[0].resumeRelevant);

    // Unknown duration does not fabricate completion while canonical resume relevance can still be known.
    assert(service.recordActivity("actor-a", "default", "r2", 17, true, true, true, false, "op-unknown"));
    items = service.list("actor-a", "default");
    bool sawUnknown = false;
    for (const auto& item : items) {
        if (item.recording.recordingId == "r2") {
            sawUnknown = true;
            assert(!item.completionKnown);
            assert(item.resumeRelevanceKnown && item.resumeRelevant);
        }
    }
    assert(sawUnknown);

    // Actor/backend scopes are isolated.
    assert(service.list("actor-b", "default").empty());
    assert(service.list("actor-a", "other").empty());
    assert(service.recordActivity("actor-a", "other", "r1", 9, true, true, true, false, "op-other"));
    assert(service.list("actor-a", "other").size() == 1);

    // Duplicate operation replay is idempotent and does not rewrite activity state.
    assert(service.recordActivity("actor-a", "default", "r2", 23, true, true, true, false, "op-repeat"));
    assert(service.recordActivity("actor-a", "default", "r2", 77, true, true, true, false, "op-repeat"));
    items = service.list("actor-a", "default");
    for (const auto& item : items) {
        if (item.recording.recordingId == "r2") assert(item.positionSeconds == 23);
    }

    // Activity ordering follows accepted playback evidence.
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    assert(service.recordActivity("actor-a", "default", "r1", 50, true, true, true, false, "op-newer"));
    items = service.list("actor-a", "default");
    assert(items.size() == 2);
    assert(items[0].recording.recordingId == "r1");

    // Missing/deleted Recording truth is not exposed and stale history is cleaned.
    recordings.erase("default:r1");
    items = service.list("actor-a", "default");
    assert(items.size() == 1);
    assert(items[0].recording.recordingId == "r2");

    // Retention is bounded per actor/backend instead of becoming an event log.
    for (int index = 0; index < 105; ++index) {
        const std::string id = "bulk-" + std::to_string(index);
        put(recording("default", id, 100));
        assert(service.recordActivity(
            "actor-limit", "default", id, index + 1, true, true, true, false,
            "op-bulk-" + std::to_string(index)));
    }
    assert(repository.findForActorBackend("actor-limit", "default").size() ==
        static_cast<std::size_t>(RecentlyWatchedRepository::MaxItemsPerActorBackend));

    // Unsupported identity never creates History truth.
    assert(!service.recordActivity("actor-a", "default", "missing", 1, true, true, true, false, "op-missing"));

    return 0;
}
