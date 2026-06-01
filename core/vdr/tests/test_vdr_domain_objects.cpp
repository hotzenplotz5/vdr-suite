#include <cassert>

#include "../include/VdrChannel.h"
#include "../include/VdrEvent.h"
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

    assert(channel.id == "1");
    assert(channel.number == 1);
    assert(channel.name == "Das Erste");
    assert(channel.provider == "ARD");
    assert(channel.group == "TV");
    assert(channel.radio == false);
    assert(channel.encrypted == false);
    assert(channel.enabled == true);

    VdrEvent event;
    event.id = "200";
    event.channelId = "1";
    event.title = "Tagesschau";
    event.subtitle = "20 Uhr";
    event.description = "Nachrichten des Tages";
    event.startTime = "2026-06-01T20:00:00";
    event.endTime = "2026-06-01T20:15:00";
    event.durationSeconds = 900;
    event.contentDescriptors.push_back("news");
    event.contentDescriptors.push_back("current affairs");
    event.parentalRating = 0;

    assert(event.id == "200");
    assert(event.channelId == "1");
    assert(event.title == "Tagesschau");
    assert(event.subtitle == "20 Uhr");
    assert(event.description == "Nachrichten des Tages");
    assert(event.startTime == "2026-06-01T20:00:00");
    assert(event.endTime == "2026-06-01T20:15:00");
    assert(event.durationSeconds == 900);
    assert(event.contentDescriptors.size() == 2);
    assert(event.contentDescriptors[0] == "news");
    assert(event.contentDescriptors[1] == "current affairs");
    assert(event.parentalRating == 0);

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

    assert(timer.id == "100");
    assert(timer.channelId == "1");
    assert(timer.eventId == "200");
    assert(timer.title == "Tagesschau");
    assert(timer.subtitle == "20 Uhr");
    assert(timer.startTime == "2026-06-01T20:00:00");
    assert(timer.endTime == "2026-06-01T20:15:00");
    assert(timer.priority == 50);
    assert(timer.lifetime == 99);
    assert(timer.enabled == true);
    assert(timer.recording == false);

    VdrRecording recording;
    recording.id = "500";
    recording.title = "Tatort";
    recording.path = "/video/Tatort";
    recording.startTime = "2026-06-01T20:15:00";
    recording.durationSeconds = 5400;
    recording.sizeMb = 4200;

    assert(recording.id == "500");
    assert(recording.title == "Tatort");
    assert(recording.path == "/video/Tatort");
    assert(recording.startTime == "2026-06-01T20:15:00");
    assert(recording.durationSeconds == 5400);
    assert(recording.sizeMb == 4200);

    return 0;
}
