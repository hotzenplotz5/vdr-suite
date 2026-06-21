#include "VdrSnapshotReadJsonSerializer.h"
#include "SearchTimer.h"
#include "VdrSnapshot.h"

#include <cassert>
#include <iostream>
#include <string>


static VdrSnapshot makeSnapshot(
    const std::string& backendId,
    std::size_t channelCount,
    std::size_t eventCount,
    std::size_t timerCount,
    std::size_t recordingCount)
{
    VdrSnapshot snapshot;
    snapshot.backendId = backendId;

    for (std::size_t index = 0; index < channelCount; ++index)
    {
        VdrChannel channel;
        channel.id = backendId + "-channel-" + std::to_string(index);
        snapshot.channels.push_back(channel);
    }

    for (std::size_t index = 0; index < eventCount; ++index)
    {
        VdrEvent event;
        event.id = backendId + "-event-" + std::to_string(index);
        snapshot.events.push_back(event);
    }

    for (std::size_t index = 0; index < timerCount; ++index)
    {
        VdrTimer timer;
        timer.id = backendId + "-timer-" + std::to_string(index);
        snapshot.timers.push_back(timer);
    }

    for (std::size_t index = 0; index < recordingCount; ++index)
    {
        VdrRecording recording;
        recording.id = backendId + "-recording-" + std::to_string(index);
        snapshot.recordings.push_back(recording);
    }

    return snapshot;
}

static void test_snapshot_read_serializer_serializes_multiple_snapshot_summaries()
{
    VdrSnapshotReadJsonSerializer serializer;

    const std::string json =
        serializer.serializeSnapshots({
            makeSnapshot("home-vdr", 2, 3, 1, 4),
            makeSnapshot("parents-vdr", 5, 6, 2, 7)
        });

    assert(json.find("\"snapshots\":[") != std::string::npos);

    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"channelCount\":2") != std::string::npos);
    assert(json.find("\"eventCount\":3") != std::string::npos);
    assert(json.find("\"timerCount\":1") != std::string::npos);
    assert(json.find("\"recordingCount\":4") != std::string::npos);

    assert(json.find("\"backendId\":\"parents-vdr\"") != std::string::npos);
    assert(json.find("\"channelCount\":5") != std::string::npos);
    assert(json.find("\"eventCount\":6") != std::string::npos);
    assert(json.find("\"timerCount\":2") != std::string::npos);
    assert(json.find("\"recordingCount\":7") != std::string::npos);
}


static void test_snapshot_read_serializer_serializes_status()
{
    VdrStatus status;
    status.enabled = true;
    status.mode = "snapshot-read";
    status.host = "vdr-host";
    status.port = 8002;
    status.state = "connected";

    VdrSnapshotReadJsonSerializer serializer;
    std::string json = serializer.serializeStatus(status);

    assert(json.find("\"enabled\":true") != std::string::npos);
    assert(json.find("\"mode\":\"snapshot-read\"") != std::string::npos);
    assert(json.find("\"host\":\"vdr-host\"") != std::string::npos);
    assert(json.find("\"port\":8002") != std::string::npos);
    assert(json.find("\"state\":\"connected\"") != std::string::npos);

    std::cout << json << std::endl;
}

static void test_snapshot_read_serializer_serializes_search_timers()
{
    VdrSnapshotReadJsonSerializer serializer;

    SearchTimer searchTimer = SearchTimer::create(
        SearchTimerId::fromBackendNativeId("home-vdr", "searchtimer-1"),
        "Terra X Suche",
        "Terra X",
        SearchTimerState::Active);

    const std::string json =
        serializer.serializeSearchTimers({searchTimer});

    assert(json.find("\"searchTimers\":[") != std::string::npos);
    assert(json.find("\"backendId\":\"home-vdr\"") != std::string::npos);
    assert(json.find("\"backendNativeId\":\"searchtimer-1\"") != std::string::npos);
    assert(json.find("\"name\":\"Terra X Suche\"") != std::string::npos);
    assert(json.find("\"query\":\"Terra X\"") != std::string::npos);
    assert(json.find("\"active\":true") != std::string::npos);
}

static void test_snapshot_read_serializer_serializes_empty_domain_lists()
{
    VdrSnapshotReadJsonSerializer serializer;

    assert(serializer.serializeRecordings({}) == "{\"recordings\":[]}");
    assert(serializer.serializeTimers({}) == "{\"timers\":[]}");
    assert(serializer.serializeSearchTimers({}) == "{\"searchTimers\":[]}");
    assert(serializer.serializeChannels({}) == "{\"channels\":[]}");
    assert(serializer.serializeEvents({}) == "{\"events\":[]}");
}

int main()
{
    test_snapshot_read_serializer_serializes_status();
    test_snapshot_read_serializer_serializes_empty_domain_lists();
    test_snapshot_read_serializer_serializes_search_timers();
    test_snapshot_read_serializer_serializes_multiple_snapshot_summaries();

    std::cout
        << "test_vdr_snapshot_read_json_serializer passed"
        << std::endl;

    return 0;
}
