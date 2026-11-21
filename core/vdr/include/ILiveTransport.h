#ifndef I_LIVE_TRANSPORT_H
#define I_LIVE_TRANSPORT_H

#include "LiveUpdateEvent.h"

#include <memory>
#include <string>

class BrowserLiveEventSubscription;

class ILiveTransport {
public:
    virtual ~ILiveTransport() = default;

    virtual void publish(const LiveUpdateEvent& event) = 0;
    virtual std::string stream() const = 0;
    virtual std::string replay(const std::string& lastEventId) const {
        (void)lastEventId;
        return stream();
    }
    // Optional authorized HTTP streaming upgrade; existing transport fakes
    // retain finite replay behavior without a new parallel transport API.
    virtual std::shared_ptr<BrowserLiveEventSubscription> subscribeBrowser(
        const std::string& lastEventId) const {
        (void)lastEventId;
        return nullptr;
    }
    virtual bool empty() const = 0;
    virtual void clear() = 0;
};

#endif
