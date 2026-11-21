#include "SuiteBridgeOsdFrameBuffer.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{
namespace
{

constexpr std::uint64_t FnvOffset = 1469598103934665603ULL;
constexpr std::uint64_t FnvPrime = 1099511628211ULL;

void mixByte(std::uint64_t& value, unsigned char byte)
{
    value ^= static_cast<std::uint64_t>(byte);
    value *= FnvPrime;
}

void mixString(std::uint64_t& value, const std::string& text)
{
    for (const unsigned char byte : text)
    {
        mixByte(value, byte);
    }
    mixByte(value, 0xFF);
}

void mixUnsigned(std::uint64_t& value, std::uint64_t number)
{
    for (unsigned int shift = 0; shift < 64; shift += 8)
    {
        mixByte(
            value,
            static_cast<unsigned char>((number >> shift) & 0xFFU));
    }
}

void mixSigned(std::uint64_t& value, std::int64_t number)
{
    mixUnsigned(value, static_cast<std::uint64_t>(number));
}

void mixBool(std::uint64_t& value, bool flag)
{
    mixByte(value, flag ? 1U : 0U);
}

bool validIdentity(const OsdFrame& frame)
{
    return !frame.surface.backendId.empty() &&
        frame.surface.backendId.size() <= 128 &&
        !frame.surface.surfaceId.empty() &&
        frame.surface.surfaceId.size() <= 128;
}

}

void SuiteBridgeOsdFrameBuffer::clear()
{
    snapshot_ = SuiteBridgeOsdFrameBufferSnapshot{};
}

const SuiteBridgeOsdFrameBufferSnapshot&
SuiteBridgeOsdFrameBuffer::snapshot() const
{
    return snapshot_;
}

bool SuiteBridgeOsdFrameBuffer::degraded(
    const SuiteBridgeOsdObservedFrame& observed)
{
    return !observed.sourceConsistent ||
        !observed.frame.complete ||
        observed.frame.state == OsdSurfaceState::Degraded;
}

std::uint64_t SuiteBridgeOsdFrameBuffer::fingerprint(
    const OsdFrame& frame)
{
    std::uint64_t value = FnvOffset;

    mixUnsigned(
        value,
        static_cast<std::uint64_t>(frame.state));
    mixUnsigned(
        value,
        static_cast<std::uint64_t>(frame.kind));
    mixString(value, frame.surface.osdEpoch);
    mixString(value, frame.title);
    mixString(value, frame.statusMessage);
    mixString(value, frame.red);
    mixString(value, frame.green);
    mixString(value, frame.yellow);
    mixString(value, frame.blue);
    mixSigned(value, frame.selectedIndex);
    mixString(value, frame.text);
    mixString(value, frame.channel);
    mixSigned(value, frame.programme.presentTime);
    mixString(value, frame.programme.presentTitle);
    mixString(value, frame.programme.presentSubtitle);
    mixSigned(value, frame.programme.followingTime);
    mixString(value, frame.programme.followingTitle);
    mixString(value, frame.programme.followingSubtitle);

    for (const OsdTextItem& item : frame.items)
    {
        mixString(value, item.text);
        mixBool(value, item.selectable);
    }

    return value;
}

SuiteBridgeOsdFrameBufferApplyResult
SuiteBridgeOsdFrameBuffer::apply(
    const SuiteBridgeOsdObservedFrame& observed)
{
    const OsdFrame& next = observed.frame;

    if (!validIdentity(next) ||
        !next.fullFrame ||
        !osdFrameWithinLimits(next))
    {
        snapshot_.resyncRequired = true;
        return SuiteBridgeOsdFrameBufferApplyResult::Rejected;
    }

    const std::uint64_t nextFingerprint = fingerprint(next);

    if (!snapshot_.hasFrame)
    {
        snapshot_.hasFrame = true;
        snapshot_.observed = observed;
        snapshot_.contentFingerprint = nextFingerprint;
        snapshot_.resyncRequired = degraded(observed);
        return snapshot_.resyncRequired
            ? SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired
            : SuiteBridgeOsdFrameBufferApplyResult::Adopted;
    }

    const OsdFrame& previous = snapshot_.observed.frame;

    if (previous.surface.backendId != next.surface.backendId ||
        previous.surface.surfaceId != next.surface.surfaceId)
    {
        snapshot_.resyncRequired = true;
        return SuiteBridgeOsdFrameBufferApplyResult::Rejected;
    }

    if (previous.surface.backendGeneration !=
        next.surface.backendGeneration)
    {
        snapshot_.observed = observed;
        snapshot_.contentFingerprint = nextFingerprint;
        snapshot_.resyncRequired = degraded(observed);
        return snapshot_.resyncRequired
            ? SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired
            : SuiteBridgeOsdFrameBufferApplyResult::ReplacedBackendGeneration;
    }

    if (next.state == OsdSurfaceState::Inactive)
    {
        snapshot_.observed = observed;
        snapshot_.contentFingerprint = nextFingerprint;
        snapshot_.resyncRequired = false;
        return SuiteBridgeOsdFrameBufferApplyResult::BecameInactive;
    }

    if (previous.state == OsdSurfaceState::Inactive ||
        previous.surface.osdEpoch != next.surface.osdEpoch)
    {
        snapshot_.observed = observed;
        snapshot_.contentFingerprint = nextFingerprint;
        snapshot_.resyncRequired = degraded(observed);
        return snapshot_.resyncRequired
            ? SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired
            : SuiteBridgeOsdFrameBufferApplyResult::ReplacedEpoch;
    }

    if (next.frameSequence < previous.frameSequence)
    {
        snapshot_.resyncRequired = true;
        return SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired;
    }

    if (next.frameSequence == previous.frameSequence)
    {
        if (nextFingerprint == snapshot_.contentFingerprint &&
            observed.droppedUpdates ==
                snapshot_.observed.droppedUpdates &&
            observed.sourceConsistent ==
                snapshot_.observed.sourceConsistent)
        {
            return SuiteBridgeOsdFrameBufferApplyResult::Duplicate;
        }

        snapshot_.resyncRequired = true;
        return SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired;
    }

    const bool continuityLost =
        observed.droppedUpdates >
            snapshot_.observed.droppedUpdates ||
        degraded(observed);

    snapshot_.observed = observed;
    snapshot_.contentFingerprint = nextFingerprint;

    if (snapshot_.resyncRequired || continuityLost)
    {
        snapshot_.resyncRequired = true;
        return SuiteBridgeOsdFrameBufferApplyResult::ResyncRequired;
    }

    return SuiteBridgeOsdFrameBufferApplyResult::Updated;
}

}
