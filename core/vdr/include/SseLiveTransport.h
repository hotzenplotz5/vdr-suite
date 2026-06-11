#ifndef SSE_LIVE_TRANSPORT_H
#define SSE_LIVE_TRANSPORT_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"
#include "LiveUpdateEventJsonSerializer.h"

#include <string>
#include <vector>

class SseLiveTransport : public ILiveTransport {
public:
    void publish(
        const LiveUpdateEvent& event) override;

    const std::vector<std::string>& frames() const;
    std::string stream() const override;
    bool empty() const override;
    void clear() override;

private:
    std::vector<std::string> frames_;
    LiveUpdateEventJsonSerializer serializer_;
};

#endif
