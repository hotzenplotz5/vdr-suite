#pragma once

#include "SuiteBridgeHandshake.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace vdrsuite::agent
{

using SuiteBridgeObservationClock = std::chrono::steady_clock;
using SuiteBridgeObservationTimePoint =
    SuiteBridgeObservationClock::time_point;

enum class SuiteBridgeObservationState
{
    NotConfigured,
    Connecting,
    PluginMissing,
    LegacyOrUnknown,
    Incompatible,
    Compatible,
    SnapshotCurrent,
    SnapshotStale,
    TransportDegraded,
    Overflowed,
    Offline
};

const char* suiteBridgeObservationStateName(
    SuiteBridgeObservationState state);

struct SuiteBridgeObservationConfig
{
    std::chrono::milliseconds pollInterval{5000};
    std::chrono::milliseconds staleAfter{15000};
    std::chrono::milliseconds offlineAfter{60000};
    std::chrono::milliseconds reconnectInitial{1000};
    std::chrono::milliseconds reconnectMaximum{30000};
};

struct SuiteBridgeSnapshotDelta
{
    std::uint64_t total = 0;
    std::uint64_t channelSwitch = 0;
    std::uint64_t recording = 0;
    std::uint64_t replaying = 0;
    std::uint64_t timerChange = 0;
};

struct SuiteBridgeObservationSnapshot
{
    SuiteBridgeObservationState state =
        SuiteBridgeObservationState::NotConfigured;
    std::string diagnostic;
    bool started = false;
    bool hasDiscovery = false;
    SuiteBridgeDiscovery discovery;
    bool hasBaseline = false;
    SuiteBridgeSnapshotBaseline baseline;
    bool hasDelta = false;
    SuiteBridgeSnapshotDelta delta;
    unsigned int consecutiveFailures = 0;
    std::optional<SuiteBridgeObservationTimePoint> lastAttemptAt;
    std::optional<SuiteBridgeObservationTimePoint> lastSuccessAt;
    std::optional<SuiteBridgeObservationTimePoint> lastStateChangeAt;
    std::optional<SuiteBridgeObservationTimePoint> nextAttemptAt;
    bool mutationsEnabled = false;
};

}