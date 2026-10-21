#ifndef SSE_LIVE_TRANSPORT_H
#define SSE_LIVE_TRANSPORT_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"
#include "LiveUpdateEventJsonSerializer.h"

#include <memory>
#include <string>
#include <vector>

class DeviceLiveEventHub;

class SseLiveTransport : public ILiveTransport {
public:
    void setDeviceLiveEventHub(std::shared_ptr<DeviceLiveEventHub> hub) {
        deviceLiveEventHub_ = std::move(hub);
    }
    void publish(
        const LiveUpdateEvent& event) override;

    const std::vector<std::string>& frames() const;
    std::string stream() const override;
    bool empty() const override;
    void clear() override;

private:
    std::vector<std::string> frames_;
    LiveUpdateEventJsonSerializer serializer_;
    std::shared_ptr<DeviceLiveEventHub> deviceLiveEventHub_;
};

#endif
