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
    value.backendNativeId = "/recordings/" + recordingId + ".rec";
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
    auto resolveNative = [&](const std::string& backendId, const std::string& backendNativeId)
        -> std::optional<RecentlyWatchedRecordingTruth> {
        for (const auto& entry : recordings) {
            if (entry.second.backendId == backendId &&
                entry.second.backendNativeId == backendNativeId)
                return entry.second;
        }
        return std::nullopt;
    };

    RecentlyWatchedService service(repository, resolve, resolveNative);

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

    // A changed backend list number must still resolve the same Recording
    // through its persisted native identity.
    auto renumbered = recording("default", "r1-new", 100);
    renumbered.backendNativeId = "/recordings/r1.rec";
    recordings.erase("default:r1");
    put(renumbered);
    items = service.list("actor-a", "default");
    bool sawRenumbered = false;
    for (const auto& item : items) {
        if (item.recording.recordingId == "r1-new") sawRenumbered = true;
    }
    assert(sawRenumbered);

    // Missing/deleted native identity is not exposed and stale history is cleaned.
    recordings.erase("default:r1-new");
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

    // Existing pre-migration rows have no native identity evidence. They must
    // not be relabelled as whichever Recording happens to reuse that number.
    Database legacyDatabase;
    assert(legacyDatabase.open(":memory:"));
    assert(legacyDatabase.execute(
        "CREATE TABLE recently_watched_state("
        "actor_id TEXT NOT NULL, backend_id TEXT NOT NULL, recording_id TEXT NOT NULL, "
        "position_seconds INTEGER NOT NULL, position_known INTEGER NOT NULL, "
        "completion_known INTEGER NOT NULL, completed INTEGER NOT NULL, "
        "resume_relevance_known INTEGER NOT NULL, resume_relevant INTEGER NOT NULL, "
        "source_evidence TEXT NOT NULL, last_activity_at TEXT NOT NULL, "
        "last_operation_id TEXT NOT NULL, PRIMARY KEY(actor_id, backend_id, recording_id));"));
    assert(legacyDatabase.execute(
        "INSERT INTO recently_watched_state VALUES("
        "'legacy','default','r2',55,1,1,0,1,1,"
        "'canonical-recording-playback-owner','2026-09-04T21:45:02.580Z','legacy-op');"));
    RecentlyWatchedRepository legacyRepository(legacyDatabase);
    assert(legacyRepository.ensureSchema());
    RecentlyWatchedService legacyService(legacyRepository, resolve, resolveNative);
    assert(legacyService.list("legacy", "default").empty());
    assert(legacyRepository.findForActorBackend("legacy", "default").empty());

    return 0;
}
