#include <cassert>

#include "../include/VdrChannel.h"
#include "../include/VdrTimer.h"
#include "../include/VdrRecording.h"

int main()
{
    VdrChannel channel;
    channel.id = "1";
    channel.number = 1;
    channel.name = "Das Erste";
    channel.provider = "ARD";
    channel.group = "TV";
    channel.radio = false;
    channel.encrypted = false;
    channel.enabled = true;

    assert(channel.number == 1);
    assert(channel.name == "Das Erste");

    VdrTimer timer;
    timer.id = "100";
    timer.channelId = "1";
    timer.eventId = "200";
    timer.title = "Tagesschau";
    timer.subtitle = "20 Uhr";
    timer.startTime = "2026-06-01T20:00:00";
    timer.endTime = "2026-06-01T20:15:00";
    timer.priority = 50;
    timer.lifetime = 99;
    timer.enabled = true;
    timer.recording = false;

    assert(timer.title == "Tagesschau");
    assert(timer.priority == 50);

    VdrRecording recording;
    recording.id = "500";
    recording.title = "Tatort";
    recording.path = "/video/Tatort";
    recording.startTime = "2026-06-01T20:15:00";
    recording.durationSeconds = 5400;
    recording.sizeMb = 4200;

    assert(recording.title == "Tatort");
    assert(recording.durationSeconds == 5400);

    return 0;
}
