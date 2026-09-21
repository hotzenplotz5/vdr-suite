#include "SseLiveTransport.h"

#include <sstream>

void SseLiveTransport::publish(
    const LiveUpdateEvent& event)
{
    std::ostringstream frame;

    frame
        << "event: update\n"
        << "id: " << event.sequenceNumber() << "\n"
        << "data: " << serializer_.serializeEvent(event) << "\n"
        << "\n";

    frames_.push_back(frame.str());
}

const std::vector<std::string>& SseLiveTransport::frames() const
{
    return frames_;
}

std::string SseLiveTransport::stream() const
{
    std::ostringstream output;

    for (const auto& frame : frames_) {
        output << frame;
    }

    return output.str();
}

bool SseLiveTransport::empty() const
{
    return frames_.empty();
}

void SseLiveTransport::clear()
{
    frames_.clear();
}
