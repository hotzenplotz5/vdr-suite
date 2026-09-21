#include "VdrOverview.h"
#include "VdrOverviewJsonSerializer.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    VdrOverview overview;

    overview.status.enabled = true;
    overview.status.mode = "mock";
    overview.status.host = "mock";
    overview.status.port = 0;
    overview.status.state = "connected";

    overview.totalChannels = 3;
    overview.radioChannels = 1;
    overview.encryptedChannels = 0;

    overview.totalEvents = 2;

    overview.totalTimers = 1;
    overview.activeTimers = 1;
    overview.recordingTimers = 0;

    overview.hasNextTimer = true;
    overview.nextTimer.id = "mock-timer-1";
    overview.nextTimer.channelId = "mock-channel-1";
    overview.nextTimer.eventId = "mock-event-1";
    overview.nextTimer.title = "Tagesschau";
    overview.nextTimer.subtitle = "20 Uhr";
    overview.nextTimer.startTime = "2026-06-01T20:00:00";
    overview.nextTimer.endTime = "2026-06-01T20:15:00";
    overview.nextTimer.priority = 50;
    overview.nextTimer.lifetime = 99;
    overview.nextTimer.enabled = true;
    overview.nextTimer.recording = false;

    overview.totalRecordings = 2;

    overview.hasLatestRecording = true;
    overview.latestRecording.id = "mock-recording-1";
    overview.latestRecording.title = "Tagesschau";
    overview.latestRecording.path =
        "/Mock/Tagesschau/2026-06-01.20.00.1-0.rec";
    overview.latestRecording.startTime = "2026-06-01T20:00:00";
    overview.latestRecording.durationSeconds = 900;
    overview.latestRecording.sizeMb = 512;

    VdrOverviewJsonSerializer serializer;

    std::string json =
        serializer.serialize(overview);

    assert(json.find("\"status\"") != std::string::npos);
    assert(json.find("\"channels\"") != std::string::npos);
    assert(json.find("\"events\"") != std::string::npos);
    assert(json.find("\"timers\"") != std::string::npos);
    assert(json.find("\"recordings\"") != std::string::npos);

    assert(json.find("\"enabled\":true")
           != std::string::npos);

    assert(json.find("\"totalChannels\":3")
           != std::string::npos);

    assert(json.find("\"totalEvents\":2")
           != std::string::npos);

    assert(json.find("\"totalTimers\":1")
           != std::string::npos);

    assert(json.find("\"hasNextTimer\":true")
           != std::string::npos);

    assert(json.find("\"id\":\"mock-timer-1\"")
           != std::string::npos);

    assert(json.find("\"totalRecordings\":2")
           != std::string::npos);

    assert(json.find("\"hasLatestRecording\":true")
           != std::string::npos);

    assert(json.find("\"title\":\"Tagesschau\"")
           != std::string::npos);

    std::cout
        << json
        << std::endl;

    std::cout
        << "test_vdr_overview_json_serializer passed"
        << std::endl;

    return 0;
}
