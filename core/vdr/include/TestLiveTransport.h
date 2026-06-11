#ifndef TEST_LIVE_TRANSPORT_H
#define TEST_LIVE_TRANSPORT_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"

#include <vector>

class TestLiveTransport : public ILiveTransport {
public:
    void publish(
        const LiveUpdateEvent& event) override;

    const std::vector<LiveUpdateEvent>& events() const;
    bool empty() const;
    void clear();

private:
    std::vector<LiveUpdateEvent> events_;
};

#endif
