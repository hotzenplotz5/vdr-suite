#pragma once

#include "SuiteBridgeOsdFrameParser.h"

#include <cstdint>

namespace vdrsuite::agent
{

enum class SuiteBridgeOsdFrameBufferApplyResult
{
    Adopted,
    Updated,
    Duplicate,
    ReplacedEpoch,
    ReplacedBackendGeneration,
    BecameInactive,
    ResyncRequired,
    Rejected
};

struct SuiteBridgeOsdFrameBufferSnapshot
{
    bool hasFrame = false;
    bool resyncRequired = false;
    SuiteBridgeOsdObservedFrame observed;
    std::uint64_t contentFingerprint = 0;
};

class SuiteBridgeOsdFrameBuffer
{
public:
    SuiteBridgeOsdFrameBufferApplyResult apply(
        const SuiteBridgeOsdObservedFrame& observed);

    void clear();

    const SuiteBridgeOsdFrameBufferSnapshot& snapshot() const;

private:
    static std::uint64_t fingerprint(const OsdFrame& frame);
    static bool degraded(const SuiteBridgeOsdObservedFrame& observed);

    SuiteBridgeOsdFrameBufferSnapshot snapshot_;
};

}
