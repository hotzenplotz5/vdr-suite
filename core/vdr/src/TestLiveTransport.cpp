#include "TestLiveTransport.h"

#include <sstream>

void TestLiveTransport::publish(
    const LiveUpdateEvent& event)
{
    events_.push_back(event);
}

const std::vector<LiveUpdateEvent>& TestLiveTransport::events() const
{
    return events_;
}

std::string TestLiveTransport::stream() const
{
    std::ostringstream output;

    for (const auto& event : events_) {
        output
            << event.sequenceNumber()
            << ":"
            << event.backendId()
            << "\n";
    }

    return output.str();
}

bool TestLiveTransport::empty() const
{
    return events_.empty();
}

void TestLiveTransport::clear()
{
    events_.clear();
}
