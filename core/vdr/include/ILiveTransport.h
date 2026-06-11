#ifndef I_LIVE_TRANSPORT_H
#define I_LIVE_TRANSPORT_H

#include "LiveUpdateEvent.h"

class ILiveTransport {
public:
    virtual ~ILiveTransport() = default;

    virtual void publish(
        const LiveUpdateEvent& event) = 0;
};

#endif
