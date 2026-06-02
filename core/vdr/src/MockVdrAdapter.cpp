#include "MockVdrAdapter.h"

VdrStatus MockVdrAdapter::getStatus() const
{
    VdrStatus status;
    status.enabled = true;
    status.mode = "mock";
    status.host = "mock";
    status.port = 0;
    status.state = "connected";

    return status;
}

std::vector<VdrChannel> MockVdrAdapter::getChannels() const
{
    VdrChannel channel1;
    channel1.id = "mock-channel-1";
    channel1.number = 1;
    channel1.name = "Das Erste HD";
    channel1.provider = "ARD";
    channel1.group = "TV";
    channel1.radio = false;
    channel1.encrypted = false;
    channel1.enabled = true;

    VdrChannel channel2;
    channel2.id = "mock-channel-2";
    channel2.number = 2;
    channel2.name = "ZDF HD";
    channel2.provider = "ZDF";
    channel2.group = "TV";
    channel2.radio = false;
    channel2.encrypted = false;
    channel2.enabled = true;

    VdrChannel channel3;
    channel3.id = "mock-channel-3";
    channel3.number = 101;
    channel3.name = "Radio Hamburg";
    channel3.provider = "Radio";
    channel3.group = "Radio";
    channel3.radio = true;
    channel3.encrypted = false;
    channel3.enabled = true;

    return { channel1, channel2, channel3 };
}

std::vector<VdrEvent> MockVdrAdapter::getEvents() const
{
    VdrEvent event1;
    event1.id = "mock-event-1";
    event1.channelId = "mock-channel-1";
    event1.title = "Tagesschau";
    event1.subtitle = "20 Uhr";
    event1.description = "Nachrichten des Tages";
    event1.startTime = "2026-06-01T20:00:00";
    event1.endTime = "2026-06-01T20:15:00";
    event1.durationSeconds = 900;
    event1.contentDescriptors = { "news" };
    event1.parentalRating = 0;

    VdrEvent event2;
    event2.id = "mock-event-2";
    event2.channelId = "mock-channel-1";
    event2.title = "Tatort";
    event2.subtitle = "Borowski und das Meer";
    event2.description = "Kriminalfilm";
    event2.startTime = "2026-06-01T20:15:00";
    event2.endTime = "2026-06-01T21:45:00";
    event2.durationSeconds = 5400;
    event2.contentDescriptors = { "movie", "crime" };
    event2.parentalRating = 12;

    return { event1, event2 };
}

std::vector<VdrTimer> MockVdrAdapter::getTimers() const
{
    VdrTimer timer;
    timer.id = "mock-timer-1";
    timer.channelId = "mock-channel-1";
    timer.eventId = "mock-event-1";
    timer.title = "Tagesschau";
    timer.subtitle = "20 Uhr";
    timer.startTime = "2026-06-01T20:00:00";
    timer.endTime = "2026-06-01T20:15:00";
    timer.priority = 50;
    timer.lifetime = 99;
    timer.enabled = true;
    timer.recording = false;

    return { timer };
}

std::vector<VdrRecording> MockVdrAdapter::getRecordings() const
{
    VdrRecording recording1;
    recording1.id = "mock-recording-1";
    recording1.title = "Tagesschau";
    recording1.path = "/Mock/Tagesschau/2026-06-01.20.00.1-0.rec";
    recording1.startTime = "2026-06-01T20:00:00";
    recording1.durationSeconds = 900;
    recording1.sizeMb = 512;

    VdrRecording recording2;
    recording2.id = "mock-recording-2";
    recording2.title = "Tatort";
    recording2.path = "/Mock/Tatort/2026-06-01.20.15.1-0.rec";
    recording2.startTime = "2026-06-01T20:15:00";
    recording2.durationSeconds = 5400;
    recording2.sizeMb = 4096;

    return { recording1, recording2 };
}
