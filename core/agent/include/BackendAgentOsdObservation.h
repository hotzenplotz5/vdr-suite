#pragma once

#include "SuiteBridgeOsdFrameSource.h"

#include <cstdint>
#include <string>

// Transient, typed Agent observation. Neither provider JSON nor OSD text is
// persisted in the ordinary observation repository or accountability store.
struct BackendAgentOsdObservation
{
    static constexpr std::size_t MaximumBodyBytes = 512U * 1024U;
    static constexpr std::int64_t FreshnessSeconds = 60;
    std::string protocolVersion = "vdr-suite-agent/1";
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::uint64_t producerSequence = 0;
    vdrsuite::agent::SuiteBridgeOsdFrameSourceSnapshot snapshot;
};

struct BackendAgentOsdReadResult
{
    bool available = false;
    std::string reasonCode;
    vdrsuite::agent::SuiteBridgeOsdFrameSourceSnapshot snapshot;
};

bool validBackendAgentOsdObservation(const BackendAgentOsdObservation& value);
std::string serializeBackendAgentOsdObservation(const BackendAgentOsdObservation& value);
bool parseBackendAgentOsdObservation(const std::string& body,
                                   BackendAgentOsdObservation& value);
