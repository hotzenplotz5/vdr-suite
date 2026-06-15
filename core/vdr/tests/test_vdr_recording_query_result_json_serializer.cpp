#include "VdrRecordingQueryResultJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title)
{
    VdrRecording recording;
    recording.id = id;
    recording.backendId = "ferienhaus";
    recording.title = title;
    recording.path = "/Mock/" + title + ".rec";
    recording.startTime = "2026-06-01T20:00:00";
    recording.durationSeconds = 900;
    recording.sizeMb = 512;
    return recording;
}

int main()
{
    std::vector<VdrRecording> recordings;
    recordings.push_back(makeRecording("1", "Tatort"));
    recordings.push_back(makeRecording("2", "Tagesschau"));

    VdrRecordingQueryResult result(
        recordings,
        973,
        2,
        10);

    VdrRecordingQueryResultJsonSerializer serializer;
    std::string json =
        serializer.serialize(result);

    assert(json.find("\"totalCount\":973") != std::string::npos);
    assert(json.find("\"returnedCount\":2") != std::string::npos);
    assert(json.find("\"limit\":2") != std::string::npos);
    assert(json.find("\"offset\":10") != std::string::npos);
    assert(json.find("\"recordings\":[") != std::string::npos);
    assert(json.find("\"id\":\"1\"") != std::string::npos);
    assert(json.find("\"backendId\":\"ferienhaus\"") != std::string::npos);
    assert(json.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(json.find("\"path\":\"/Mock/Tatort.rec\"") != std::string::npos);
    assert(json.find("\"durationSeconds\":900") != std::string::npos);
    assert(json.find("\"sizeMb\":512") != std::string::npos);
    assert(json.find("\"id\":\"2\"") != std::string::npos);
    assert(json.find("\"title\":\"Tagesschau\"") != std::string::npos);

    VdrRecordingQueryResult empty =
        VdrRecordingQueryResult::empty(
            50,
            100);

    std::string emptyJson =
        serializer.serialize(empty);

    assert(emptyJson == "{\"totalCount\":0,\"returnedCount\":0,\"limit\":50,\"offset\":100,\"recordings\":[]}");

    std::cout
        << "test_vdr_recording_query_result_json_serializer passed"
        << std::endl;

    return 0;
}
