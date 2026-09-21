#ifndef I_LIVE_TRANSPORT_H
#define I_LIVE_TRANSPORT_H

#include "LiveUpdateEvent.h"

#include <string>

class ILiveTransport {
public:
    virtual ~ILiveTransport() = default;

    virtual void publish(
        const LiveUpdateEvent& event) = 0;

    virtual std::string stream() const = 0;
    virtual bool empty() const = 0;
    virtual void clear() = 0;
};

#endif
