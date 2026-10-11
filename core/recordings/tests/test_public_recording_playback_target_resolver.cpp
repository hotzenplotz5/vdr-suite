#include "PublicRecordingPlaybackTargetResolver.h"
#include "PublicRecordingIdentityRepository.h"
#include "VdrRecordingCacheRepository.h"
#include "Database.h"
#include <cassert>
#include <string>

static VdrRecording makeRecord(const std::string& id,
    const std::string& native) {
    VdrRecording r;
    r.id=id; r.backendNativeId=native;
    r.title="Echte Aufnahme"; r.path="/Movies/Test";
    r.startTime="2026-10-09T20:15:00Z";
    return r;
}
static void expect(const PublicRecordingPlaybackTargetResult& r,
    PublicRecordingPlaybackTargetStatus s) {
    assert(r.status==s);
    if (s!=PublicRecordingPlaybackTargetStatus::ready) {
        assert(r.recording.id.empty());
        assert(r.recording.backendNativeId.empty());
    }
}
int main() {
    Database db; assert(db.open(":memory:"));
    PublicRecordingIdentityRepository ids(db);
    VdrRecordingCacheRepository cache(db);
    assert(ids.ensureSchema() && cache.ensureSchema());
    const std::string native="/private/Movies/A/2026.rec";
    const std::string moved="/private/Movies/B/2026.rec";
    assert(cache.replaceRecordingsForBackend("backend-a",
        {makeRecord("media-record-17",native)}));
    const auto publicId=ids.resolveOrCreate("backend-a",native);
    assert(publicId);
    PublicRecordingPlaybackTargetResolver resolver(ids,cache);
    auto found=resolver.resolve("backend-a",*publicId);
    expect(found,PublicRecordingPlaybackTargetStatus::ready);
    assert(found.recording.id=="media-record-17");
    assert(found.recording.id!=*publicId);
    assert(found.recording.backendNativeId==native);
    assert(found.recording.backendId=="backend-a");
    expect(resolver.resolve("backend-b",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    expect(resolver.resolve("backend-a","rec_invalid"),
        PublicRecordingPlaybackTargetStatus::invalidRequest);
    expect(resolver.resolve("backend-a",std::string("rec_")+std::string(32,'A')),
        PublicRecordingPlaybackTargetStatus::invalidRequest);
    expect(resolver.resolve("",*publicId),
        PublicRecordingPlaybackTargetStatus::invalidRequest);
    expect(resolver.resolve("*",*publicId),
        PublicRecordingPlaybackTargetStatus::invalidRequest);
    expect(resolver.resolve("../backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::invalidRequest);
    expect(resolver.resolve("backend-a",std::string("rec_")+std::string(32,'0')),
        PublicRecordingPlaybackTargetStatus::notFound);
    // An identity without a current recording is not playable.
    assert(cache.replaceRecordingsForBackend("backend-a",{}));
    expect(resolver.resolve("backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    // A move requires an explicit rebind and matching new cache snapshot.
    assert(ids.rebindAfterVerifiedMove("backend-a",native,moved));
    assert(cache.replaceRecordingsForBackend("backend-a",
        {makeRecord("stale",native)}));
    expect(resolver.resolve("backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    assert(cache.replaceRecordingsForBackend("backend-a",
        {makeRecord("media-record-23",moved)}));
    found=resolver.resolve("backend-a",*publicId);
    expect(found,PublicRecordingPlaybackTargetStatus::ready);
    assert(found.recording.id=="media-record-23");
    // Must not pass a missing media-service ID to the old controller.
    assert(cache.replaceRecordingsForBackend("backend-a",
        {makeRecord("",moved)}));
    expect(resolver.resolve("backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    assert(ids.removeAfterVerifiedDeletion("backend-a",moved));
    expect(resolver.resolve("backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
}
