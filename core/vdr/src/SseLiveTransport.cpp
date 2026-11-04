#include "SseLiveTransport.h"
#include "DeviceLiveEventHub.h"

#include <charconv>
#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>

namespace {
std::string newDaemonEpoch()
{
    std::random_device random;
    std::ostringstream value;
    value << "sse_";
    for (int i = 0; i < 4; ++i)
        value << std::hex << std::setfill('0') << std::setw(8)
              << static_cast<std::uint32_t>(random());
    return value.str();
}
}

SseLiveTransport::SseLiveTransport()
    : daemonEpoch_(newDaemonEpoch()) {}

void SseLiveTransport::publish(const LiveUpdateEvent& event)
{
    const int sequence = event.sequenceNumber();
    if (sequence > latestSequenceNumber_) {
        latestSequenceNumber_ = sequence;
        std::ostringstream frame;
        frame << "event: update\n"
              << "id: " << daemonEpoch_ << ":" << sequence << "\n"
              << "data: " << serializer_.serializeEvent(event) << "\n\n";
        std::string serialized = frame.str();

        if (serialized.size() > maxRetainedBytes) {
            // One oversized event creates a detectable replay gap.
            clear();
        } else {
            while (!frames_.empty() &&
                   (frames_.size() >= maxRetainedFrames ||
                    retainedBytes_ > maxRetainedBytes - serialized.size())) {
                retainedBytes_ -= frames_.front().size();
                frames_.erase(frames_.begin());
                frameSequenceNumbers_.erase(frameSequenceNumbers_.begin());
            }
            retainedBytes_ += serialized.size();
            frames_.push_back(std::move(serialized));
            frameSequenceNumbers_.push_back(sequence);
        }
    }
    if (deviceLiveEventHub_) deviceLiveEventHub_->publish(event);
}

const std::vector<std::string>& SseLiveTransport::frames() const
{
    return frames_;
}

int SseLiveTransport::oldestSequenceNumber() const
{
    return frameSequenceNumbers_.empty() ? 0 : frameSequenceNumbers_.front();
}

std::string SseLiveTransport::stream() const
{
    std::ostringstream output;
    for (const auto& frame : frames_) output << frame;
    return output.str();
}

std::string SseLiveTransport::resyncFrame(const char* reason) const
{
    std::ostringstream output;
    output << "retry: 5000\n"
           << "event: resync\n"
           << "id: " << daemonEpoch_ << ":" << latestSequenceNumber_ << "\n"
           << "data: {\"reason\":\"" << reason
           << "\",\"latestSequenceNumber\":" << latestSequenceNumber_ << "}\n\n";
    return output.str();
}

std::string SseLiveTransport::replay(const std::string& lastEventId) const
{
    if (lastEventId.empty()) return resyncFrame("initial");

    const std::size_t separator = lastEventId.rfind(':');
    if (separator == std::string::npos || lastEventId.size() > 128 ||
        separator + 1 == lastEventId.size())
        return resyncFrame("invalid_cursor");
    if (lastEventId.substr(0, separator) != daemonEpoch_)
        return resyncFrame("epoch_changed");

    const std::string digits = lastEventId.substr(separator + 1);
    std::uint64_t cursor = 0;
    const auto parsed = std::from_chars(
        digits.data(), digits.data() + digits.size(), cursor);
    if (parsed.ec != std::errc{} ||
        parsed.ptr != digits.data() + digits.size() ||
        std::to_string(cursor) != digits ||
        cursor > static_cast<std::uint64_t>(latestSequenceNumber_))
        return resyncFrame("invalid_cursor");

    if (cursor == static_cast<std::uint64_t>(latestSequenceNumber_))
        return "retry: 5000\n\n";
    if (frames_.empty() ||
        cursor + 1 < static_cast<std::uint64_t>(oldestSequenceNumber()))
        return resyncFrame("gap");

    std::string response = "retry: 5000\n\n";
    std::uint64_t expected = cursor + 1;
    for (std::size_t i = 0; i < frames_.size(); ++i) {
        const auto sequence = static_cast<std::uint64_t>(frameSequenceNumbers_[i]);
        if (sequence <= cursor) continue;
        if (sequence != expected) return resyncFrame("gap");
        response += frames_[i];
        ++expected;
    }
    if (expected != static_cast<std::uint64_t>(latestSequenceNumber_) + 1)
        return resyncFrame("gap");
    return response;
}

bool SseLiveTransport::empty() const
{
    return frames_.empty();
}

void SseLiveTransport::clear()
{
    frames_.clear();
    frameSequenceNumbers_.clear();
    retainedBytes_ = 0;
}
