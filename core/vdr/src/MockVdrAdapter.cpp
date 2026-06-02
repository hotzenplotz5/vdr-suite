#include "MockVdrAdapter.h"

MockVdrAdapter::MockVdrAdapter()
{
}

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
    VdrChannel dasErste;
    dasErste.id = "mock-channel-1";
    dasErste.number = 1;
    dasErste.name = "Das Erste HD";
    dasErste.provider = "ARD";
    dasErste.group = "TV";
    dasErste.radio = false;
    dasErste.encrypted = false;
    dasErste.enabled = true;

    VdrChannel zdf;
    zdf.id = "mock-channel-2";
    zdf.number = 2;
    zdf.name = "ZDF HD";
    zdf.provider = "ZDF";
    zdf.group = "TV";
    zdf.radio = false;
    zdf.encrypted = false;
    zdf.enabled = true;

    VdrChannel radio;
    radio.id = "mock-channel-3";
    radio.number = 101;
    radio.name = "Radio Hamburg";
    radio.provider = "";
    radio.group = "Radio";
    radio.radio = true;
    radio.encrypted = false;
    radio.enabled = true;

    std::vector<VdrChannel> channels;
    channels.push_back(dasErste);
    channels.push_back(zdf);
    channels.push_back(radio);

    return channels;
}

std::vector<VdrEvent> MockVdrAdapter::getEvents() const
{
    VdrEvent tagesschau;
    tagesschau.id = "mock-event-1";
    tagesschau.channelId = "mock-channel-1";
    tagesschau.title = "Tagesschau";
    tagesschau.subtitle = "20 Uhr";
    tagesschau.description = "Nachrichten des Tages";
    tagesschau.startTime = "2026-06-01T20:00:00";
    tagesschau.endTime = "2026-06-01T20:15:00";
    tagesschau.durationSeconds = 900;
    tagesschau.contentDescriptors.push_back("news");
    tagesschau.parentalRating = 0;

    VdrEvent tatort;
    tatort.id = "mock-event-2";
    tatort.channelId = "mock-channel-1";
    tatort.title = "Tatort";
    tatort.subtitle = "Borowski und das Meer";
    tatort.description = "Kriminalfilm";
    tatort.startTime = "2026-06-01T20:15:00";
    tatort.endTime = "2026-06-01T21:45:00";
    tatort.durationSeconds = 5400;
    tatort.contentDescriptors.push_back("movie");
    tatort.contentDescriptors.push_back("crime");
    tatort.parentalRating = 12;

    std::vector<VdrEvent> events;
    events.push_back(tagesschau);
    events.push_back(tatort);

    return events;
}
