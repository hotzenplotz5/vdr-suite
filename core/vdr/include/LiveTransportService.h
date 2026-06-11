#ifndef LIVE_TRANSPORT_SERVICE_H
#define LIVE_TRANSPORT_SERVICE_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"
#include "SnapshotChangeFeedEntry.h"

class LiveTransportService {
public:
    explicit LiveTransportService(
        ILiveTransport& transport);

    void publish(
        const LiveUpdateEvent& event);

    void publishChangeFeedEntry(
        const SnapshotChangeFeedEntry& entry);

private:
    ILiveTransport& transport_;
};

#endif
