#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

enum class SuiteBridgeCapabilityState
{
    Available,
    Disabled,
    Unavailable,
    Unknown
};

struct SuiteBridgeCapabilityObservation
{
    std::string id;
    SuiteBridgeCapabilityState state =
        SuiteBridgeCapabilityState::Unknown;
    std::string rawState;
};

struct SuiteBridgeDiscovery
{
    std::uint64_t discoverySchema = 0;
    std::string pluginName;
    std::string pluginVersion;
    std::uint64_t capabilitySchema = 0;
    std::uint64_t snapshotSchema = 0;
    std::uint64_t localContractSchema = 0;
    std::vector<SuiteBridgeCapabilityObservation> capabilities;

    SuiteBridgeCapabilityState capabilityState(
        const std::string& id) const;

    bool capabilityAvailable(
        const std::string& id) const;
};

struct SuiteBridgeSnapshotBaseline
{
    std::uint64_t contractSchema = 0;
    std::uint64_t capabilitySchema = 0;
    std::uint64_t snapshotSchema = 0;
    bool active = false;
    std::uint64_t total = 0;
    std::uint64_t channelSwitch = 0;
    std::uint64_t recording = 0;
    std::uint64_t replaying = 0;
    std::uint64_t timerChange = 0;
    std::string counterEpoch;
    bool counterOverflow = false;

    bool canCalculateDeltaFrom(
        const SuiteBridgeSnapshotBaseline& previous) const;
};

enum class SuiteBridgeBaselineUpdate
{
    AdoptedInitial,
    UpdatedComparable,
    ReplacedEpochChanged,
    ReplacedOverflowed
};

class SuiteBridgeBaselineTracker
{
public:
    SuiteBridgeBaselineUpdate apply(
        const SuiteBridgeSnapshotBaseline& next);

    bool hasBaseline() const;
    bool deltaAvailable() const;
    const SuiteBridgeSnapshotBaseline& baseline() const;

private:
    bool hasBaseline_ = false;
    bool deltaAvailable_ = false;
    SuiteBridgeSnapshotBaseline baseline_;
};

enum class SuiteBridgeHandshakeStatus
{
    Ready,
    LegacyOrUnknown,
    DiscoveryTransportError,
    DiscoveryReplyRejected,
    DiscoveryPayloadTooLarge,
    InvalidDiscoveryPayload,
    UnexpectedPlugin,
    IncompatibleDiscoverySchema,
    IncompatibleCapabilitySchema,
    IncompatibleSnapshotSchema,
    IncompatibleLocalContractSchema,
    RequiredCapabilityUnavailable,
    SnapshotTransportError,
    SnapshotReplyRejected,
    SnapshotPayloadTooLarge,
    InvalidSnapshotPayload,
    SnapshotInactive
};

const char* suiteBridgeHandshakeStatusName(
    SuiteBridgeHandshakeStatus status);

struct SuiteBridgeHandshakeResult
{
    SuiteBridgeHandshakeStatus status =
        SuiteBridgeHandshakeStatus::LegacyOrUnknown;
    std::string diagnostic;
    SuiteBridgeDiscovery discovery;
    SuiteBridgeSnapshotBaseline baseline;
    bool mutationsEnabled = false;

    bool ready() const
    {
        return status == SuiteBridgeHandshakeStatus::Ready;
    }
};

}
