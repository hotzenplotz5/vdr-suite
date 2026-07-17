#pragma once

#include "SuiteBridgeHandshake.h"

#include <cstddef>
#include <string>

namespace vdrsuite::agent
{

enum class SuiteBridgeParseStatus
{
    Ok,
    PayloadTooLarge,
    InvalidJson,
    MissingField,
    InvalidField
};

struct SuiteBridgeDiscoveryParseResult
{
    SuiteBridgeParseStatus status = SuiteBridgeParseStatus::InvalidJson;
    SuiteBridgeDiscovery value;
    std::string diagnostic;

    bool ok() const
    {
        return status == SuiteBridgeParseStatus::Ok;
    }
};

struct SuiteBridgeSnapshotParseResult
{
    SuiteBridgeParseStatus status = SuiteBridgeParseStatus::InvalidJson;
    SuiteBridgeSnapshotBaseline value;
    std::string diagnostic;

    bool ok() const
    {
        return status == SuiteBridgeParseStatus::Ok;
    }
};

class SuiteBridgeLocalContractParser
{
public:
    static constexpr std::size_t MaximumPayloadBytes = 4096;
    static constexpr std::size_t MaximumCapabilities = 64;

    SuiteBridgeDiscoveryParseResult parseDiscovery(
        const std::string& payload) const;

    SuiteBridgeSnapshotParseResult parseSnapshot(
        const std::string& payload) const;
};

}
