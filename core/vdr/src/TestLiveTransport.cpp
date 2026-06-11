#include "TestLiveTransport.h"

void TestLiveTransport::publish(
    const LiveUpdateEvent& event)
{
    events_.push_back(event);
}

const std::vector<LiveUpdateEvent>& TestLiveTransport::events() const
{
    return events_;
}

bool TestLiveTransport::empty() const
{
    return events_.empty();
}

void TestLiveTransport::clear()
{
    events_.clear();
}
