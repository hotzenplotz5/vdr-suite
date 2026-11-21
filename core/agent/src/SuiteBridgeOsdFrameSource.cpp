#include "SuiteBridgeOsdFrameSource.h"

#include <utility>

namespace vdrsuite::agent
{

SuiteBridgeOsdFrameSource::SuiteBridgeOsdFrameSource(
    ISuiteBridgeLocalTransport& transport,
    std::string backendId,
    std::uint64_t backendGeneration)
    : transport_(transport),
      backendId_(std::move(backendId)),
      backendGeneration_(backendGeneration)
{
}

const SuiteBridgeOsdFrameSourceSnapshot&
SuiteBridgeOsdFrameSource::snapshot() const
{
    return snapshot_;
}

void SuiteBridgeOsdFrameSource::clear()
{
    buffer_.clear();
    snapshot_ = SuiteBridgeOsdFrameSourceSnapshot{};
}

SuiteBridgeOsdFrameSourceSnapshot SuiteBridgeOsdFrameSource::read(
    const SuiteBridgeDiscovery& discovery)
{
    if (!discovery.capabilityAvailable("osd.view"))
    {
        snapshot_.state =
            SuiteBridgeOsdFrameSourceState::CapabilityUnavailable;
        snapshot_.diagnostic =
            "Suite Bridge osd.view capability unavailable";
        snapshot_.buffer = buffer_.snapshot();
        return snapshot_;
    }

    const SuiteBridgeCommandReply reply =
        transport_.execute(SuiteBridgeLocalCommand::OsdSnapshot);

    if (!reply.transportSucceeded())
    {
        snapshot_.state =
            SuiteBridgeOsdFrameSourceState::TransportDegraded;
        snapshot_.diagnostic = reply.diagnostic.empty()
            ? "Suite Bridge OSD snapshot transport failed"
            : reply.diagnostic;
        snapshot_.buffer = buffer_.snapshot();
        return snapshot_;
    }

    if (reply.replyCode != 900)
    {
        snapshot_.state =
            SuiteBridgeOsdFrameSourceState::ReplyRejected;
        snapshot_.diagnostic =
            "Suite Bridge OSD snapshot reply rejected";
        snapshot_.buffer = buffer_.snapshot();
        return snapshot_;
    }

    SuiteBridgeOsdFrameParseResult parsed = parser_.parse(
        reply.payload,
        backendId_,
        backendGeneration_);

    if (!parsed.ok())
    {
        snapshot_.state =
            SuiteBridgeOsdFrameSourceState::InvalidPayload;
        snapshot_.diagnostic = parsed.diagnostic.empty()
            ? "invalid Suite Bridge OSD snapshot payload"
            : parsed.diagnostic;
        snapshot_.buffer = buffer_.snapshot();
        return snapshot_;
    }

    const SuiteBridgeOsdFrameBufferApplyResult applied =
        buffer_.apply(parsed.value);
    snapshot_.buffer = buffer_.snapshot();

    if (applied == SuiteBridgeOsdFrameBufferApplyResult::Rejected ||
        snapshot_.buffer.resyncRequired)
    {
        snapshot_.state =
            SuiteBridgeOsdFrameSourceState::ResyncRequired;
        snapshot_.diagnostic =
            "OSD full-frame resynchronization required";
        return snapshot_;
    }

    snapshot_.state = SuiteBridgeOsdFrameSourceState::Current;
    snapshot_.diagnostic.clear();
    return snapshot_;
}

}
