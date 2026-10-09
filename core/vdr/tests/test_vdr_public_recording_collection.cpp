#include "VdrPublicRecordingCollection.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{
VdrRecording record(const std::string& backend, const std::string& address,
                    const std::string& name, int duration)
{
    VdrRecording recording;
    recording.backendId = backend;
    recording.backendNativeId = address;
    recording.path = "/srv/vdr/video/very-private-path";
    recording.id = "native-counter-not-stable";
    recording.title = name;
    recording.startTime = "2026-10-09T20:15:00";
    recording.durationSeconds = duration;
    recording.recordingDurationKnown = duration > 0;
    return recording;
}

void authorizationAndRedaction()
{
    const std::string a = "rec_00000000000000000000000000000001";
    const std::string b = "rec_00000000000000000000000000000002";
    const auto projected = projectPublicRecordings({
        record("A", "/secret/private-a.rec", "Show \"One\"", 3600),
        record("B", "/secret/private-b.rec", "Private B", 3800)
    }, {
        {a, "A", "/secret/private-a.rec"},
        {b, "B", "/secret/private-b.rec"}
    }, {"A"});
    assert(projected.valid);
    assert(projected.items.size() == 1);
    assert(projected.items[0].recordingId == a);
    assert(projected.items[0].durationSeconds == 3600);
    const auto json = serializePublicRecordingCollection(projected);
    assert(json.find("Show \\\"One\\\"") != std::string::npos);
    assert(json.find("Private B") == std::string::npos);
    assert(json.find("native-counter") == std::string::npos);
    assert(json.find("/secret/") == std::string::npos);
    assert(json.find("/srv/vdr/") == std::string::npos);
    assert(json.find("\"partial\":false") != std::string::npos);
}

void refuseAmbiguousOrUnresolvedIdentity()
{
    const auto a = record("A", "/secret/a.rec", "A", 200);
    const auto result = projectPublicRecordings(
        {a}, {}, {"A"});
    assert(!result.valid);
    assert(serializePublicRecordingCollection(result).empty());
    const auto duplicate = projectPublicRecordings(
        {a, a},
        {{"rec_00000000000000000000000000000001", "A", "/secret/a.rec"}},
        {"A"});
    assert(!duplicate.valid);
    assert(duplicate.items.empty());
    const auto badToken = projectPublicRecordings(
        {a},
        {{"../../etc/passwd", "A", "/secret/a.rec"}},
        {"A"});
    assert(!badToken.valid);
}

void emptyAuthorizedSetRevealsNothing()
{
    const auto a = record("A", "/secret/a.rec", "A", 0);
    const auto result = projectPublicRecordings(
        {a},
        {{"rec_00000000000000000000000000000001", "A", "/secret/a.rec"}},
        {});
    assert(result.valid);
    assert(result.items.empty());
}

void unknownDurationStaysUnknown()
{
    const auto a = record("A", "/secret/a.rec", "A", 0);
    const auto result = projectPublicRecordings(
        {a},
        {{"rec_00000000000000000000000000000001", "A", "/secret/a.rec"}},
        {"A"});
    assert(result.valid && result.items.size() == 1);
    assert(!result.items[0].durationKnown);
    assert(result.items[0].durationSeconds == 0);
}
}

int main()
{
    authorizationAndRedaction();
    refuseAmbiguousOrUnresolvedIdentity();
    emptyAuthorizedSetRevealsNothing();
    unknownDurationStaysUnknown();
    std::cout << "public Recording collection projection: PASS" << std::endl;
}
