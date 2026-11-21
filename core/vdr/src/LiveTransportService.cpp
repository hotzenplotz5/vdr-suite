#include "LiveTransportService.h"

LiveTransportService::LiveTransportService(
    ILiveTransport& transport)
    : transport_(transport)
{
}

void LiveTransportService::publish(
    const LiveUpdateEvent& event)
{
    if (event.hasChanges()) {
        transport_.publish(event);
    }
}

void LiveTransportService::publishChangeFeedEntry(
    const SnapshotChangeFeedEntry& entry)
{
    publish(LiveUpdateEvent(entry));
}
