#include "VdrRecordingQueryResult.h"

#include <cassert>
#include <iostream>
#include <vector>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title)
{
    VdrRecording recording;
    recording.id = id;
    recording.title = title;
    recording.path = "/Mock/" + title + ".rec";
    recording.startTime = "2026-06-01T20:00:00";
    recording.durationSeconds = 900;
    recording.sizeMb = 512;
    return recording;
}

int main()
{
    VdrRecordingQueryResult empty =
        VdrRecordingQueryResult::empty(
            50,
            100);

    assert(empty.empty());
    assert(empty.recordings().empty());
    assert(empty.totalCount() == 0);
    assert(empty.returnedCount() == 0);
    assert(empty.limit() == 50);
    assert(empty.offset() == 100);

    std::vector<VdrRecording> recordings;
    recordings.push_back(makeRecording("1", "Tatort"));
    recordings.push_back(makeRecording("2", "Tagesschau"));

    VdrRecordingQueryResult result(
        recordings,
        973,
        2,
        10);

    assert(!result.empty());
    assert(result.recordings().size() == 2);
    assert(result.returnedCount() == 2);
    assert(result.totalCount() == 973);
    assert(result.limit() == 2);
    assert(result.offset() == 10);
    assert(result.recordings().at(0).id == "1");
    assert(result.recordings().at(0).title == "Tatort");
    assert(result.recordings().at(1).id == "2");
    assert(result.recordings().at(1).title == "Tagesschau");

    std::cout
        << "test_vdr_recording_query_result passed"
        << std::endl;

    return 0;
}
