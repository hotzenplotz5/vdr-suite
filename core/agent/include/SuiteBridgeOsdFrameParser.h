#pragma once

#include "LegacyOsdDomain.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class SuiteBridgeOsdFrameParseStatus
{
    Ok,
    PayloadTooLarge,
    InvalidJson,
    UnsupportedSchema,
    InvalidFrame
};

struct SuiteBridgeOsdObservedFrame
{
    OsdFrame frame;
    bool sourceConsistent = false;
    std::uint64_t droppedUpdates = 0;
};

struct SuiteBridgeOsdFrameParseResult
{
    SuiteBridgeOsdFrameParseStatus status =
        SuiteBridgeOsdFrameParseStatus::InvalidJson;
    SuiteBridgeOsdObservedFrame value;
    std::string diagnostic;

    bool ok() const
    {
        return status == SuiteBridgeOsdFrameParseStatus::Ok;
    }
};

class SuiteBridgeOsdFrameParser
{
public:
    static constexpr std::size_t MaximumPayloadBytes = 131000;

    SuiteBridgeOsdFrameParseResult parse(
        const std::string& payload,
        const std::string& backendId,
        std::uint64_t backendGeneration) const;
};

}
