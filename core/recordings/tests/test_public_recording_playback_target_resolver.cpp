#include "PublicRecordingPlaybackTargetResolver.h"
#include "PublicRecordingActionReadback.h"
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
    // M1: an authorized metadata read is possible for legacy cache entries
    // even without a playable media-service ID or native source title.
    auto readOnly = resolver.resolveForRead("backend-a", *publicId);
    expect(readOnly, PublicRecordingPlaybackTargetStatus::ready);
    assert(readOnly.recording.id.empty());
    assert(readOnly.recording.backendNativeId == moved);
    assert(readOnly.recording.backendId == "backend-a");
    assert(cache.replaceRecordingsForBackend("backend-a",
        {makeRecord("", moved)}));
    VdrRecording titleless = makeRecord("", moved);
    titleless.title.clear();
    assert(cache.replaceRecordingsForBackend("backend-a", {titleless}));
    expect(resolver.resolve("backend-a", *publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    readOnly = resolver.resolveForRead("backend-a", *publicId);
    expect(readOnly, PublicRecordingPlaybackTargetStatus::ready);
    assert(readOnly.recording.title.empty());
    expect(resolver.resolveForRead("backend-b", *publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
    expect(resolver.resolveForRead("backend-a", "rec_invalid"),
        PublicRecordingPlaybackTargetStatus::invalidRequest);

    // Verified post-mutation provider readback, not an HTTP success alone,
    // establishes which native identity receives the existing public rec_.
    VdrRecording before = makeRecord("media-record-88",
        "/vdr/Series/Old/2026-10-09.20.15.rec");
    before.backendId = "backend-a";
    before.durationSeconds = 3600;
    before.recordingDurationKnown = true;
    before.sizeMb = 400;
    VdrRecording after = before;
    after.backendNativeId = "/vdr/Series/New/2026-10-09.20.15.rec";
    after.title = "New";
    assert(PublicRecordingActionReadback::relocatedNativeId(
        before, {after}) == std::optional<std::string>(after.backendNativeId));
    assert(!PublicRecordingActionReadback::relocatedNativeId(before, {before, after}));
    VdrRecording ambiguous = after;
    ambiguous.backendNativeId = "/vdr/Archive/New/2026-10-09.20.15.rec";
    assert(!PublicRecordingActionReadback::relocatedNativeId(
        before, {after, ambiguous}));
    VdrRecording wrongBackend = after;
    wrongBackend.backendId = "backend-b";
    assert(!PublicRecordingActionReadback::relocatedNativeId(
        before, {wrongBackend}));
    VdrRecording wrongExtent = after;
    wrongExtent.sizeMb += 1;
    assert(!PublicRecordingActionReadback::relocatedNativeId(
        before, {wrongExtent}));
    VdrRecording wrongTime = after;
    wrongTime.startTime = "2026-10-10T20:15:00Z";
    assert(!PublicRecordingActionReadback::relocatedNativeId(
        before, {wrongTime}));
    assert(!PublicRecordingActionReadback::relocatedNativeId(before, {}));
    assert(PublicRecordingActionReadback::deletedNativeId(before, {after}));
    assert(!PublicRecordingActionReadback::deletedNativeId(before, {before}));

    assert(cache.replaceRecordingsForBackend("backend-a", {before}));
    const auto stableId =
        ids.resolveOrCreate("backend-a", before.backendNativeId);
    assert(stableId);
    const auto verified = PublicRecordingActionReadback::relocatedNativeId(
        before, {after});
    assert(verified && ids.rebindAfterVerifiedMove(
        "backend-a", before.backendNativeId, *verified));
    assert(cache.replaceRecordingsForBackend("backend-a", {after}));
    const auto stableResolved = resolver.resolve("backend-a", *stableId);
    expect(stableResolved, PublicRecordingPlaybackTargetStatus::ready);
    assert(stableResolved.recording.backendNativeId == after.backendNativeId);
    assert(ids.findNativeForPublicId("backend-a", *stableId) == verified);

    assert(ids.removeAfterVerifiedDeletion("backend-a",moved));
    expect(resolver.resolve("backend-a",*publicId),
        PublicRecordingPlaybackTargetStatus::notFound);
}
