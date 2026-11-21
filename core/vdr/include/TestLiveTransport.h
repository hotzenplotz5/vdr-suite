#ifndef TEST_LIVE_TRANSPORT_H
#define TEST_LIVE_TRANSPORT_H

#include "ILiveTransport.h"
#include "LiveUpdateEvent.h"

#include <string>
#include <vector>

class TestLiveTransport : public ILiveTransport {
public:
    void publish(
        const LiveUpdateEvent& event) override;

    const std::vector<LiveUpdateEvent>& events() const;
    std::string stream() const override;
    bool empty() const override;
    void clear() override;

private:
    std::vector<LiveUpdateEvent> events_;
};

#endif
