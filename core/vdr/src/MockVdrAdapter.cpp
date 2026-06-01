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
