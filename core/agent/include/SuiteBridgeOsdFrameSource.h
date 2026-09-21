#pragma once

#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeHandshake.h"
#include "SuiteBridgeOsdFrameBuffer.h"
#include "SuiteBridgeOsdFrameParser.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class SuiteBridgeOsdFrameSourceState
{
    NotRead,
    Current,
    ResyncRequired,
    CapabilityUnavailable,
    TransportDegraded,
    ReplyRejected,
    InvalidPayload
};

struct SuiteBridgeOsdFrameSourceSnapshot
{
    SuiteBridgeOsdFrameSourceState state =
        SuiteBridgeOsdFrameSourceState::NotRead;
    std::string diagnostic;
    SuiteBridgeOsdFrameBufferSnapshot buffer;
};

class SuiteBridgeOsdFrameSource
{
public:
    SuiteBridgeOsdFrameSource(
        ISuiteBridgeLocalTransport& transport,
        std::string backendId,
        std::uint64_t backendGeneration);

    SuiteBridgeOsdFrameSourceSnapshot read(
        const SuiteBridgeDiscovery& discovery);

    const SuiteBridgeOsdFrameSourceSnapshot& snapshot() const;

    void clear();

private:
    ISuiteBridgeLocalTransport& transport_;
    std::string backendId_;
    std::uint64_t backendGeneration_ = 0;
    SuiteBridgeOsdFrameParser parser_;
    SuiteBridgeOsdFrameBuffer buffer_;
    SuiteBridgeOsdFrameSourceSnapshot snapshot_;
};

}
