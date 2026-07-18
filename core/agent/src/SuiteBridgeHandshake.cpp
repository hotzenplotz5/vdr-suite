#include "SuiteBridgeHandshake.h"

#include <stdexcept>

namespace vdrsuite::agent
{

SuiteBridgeCapabilityState SuiteBridgeDiscovery::capabilityState(
    const std::string& id) const
{
    for (const SuiteBridgeCapabilityObservation& capability : capabilities)
    {
        if (capability.id == id)
        {
            return capability.state;
        }
    }

    return SuiteBridgeCapabilityState::Unknown;
}

bool SuiteBridgeDiscovery::capabilityAvailable(
    const std::string& id) const
{
    return capabilityState(id) == SuiteBridgeCapabilityState::Available;
}

bool SuiteBridgeSnapshotBaseline::canCalculateDeltaFrom(
    const SuiteBridgeSnapshotBaseline& previous) const
{
    return active &&
           previous.active &&
           !counterOverflow &&
           !previous.counterOverflow &&
           !counterEpoch.empty() &&
           counterEpoch == previous.counterEpoch &&
           countersAtLeast(previous);
}

bool SuiteBridgeSnapshotBaseline::countersAtLeast(
    const SuiteBridgeSnapshotBaseline& previous) const
{
    return total >= previous.total &&
           channelSwitch >= previous.channelSwitch &&
           recording >= previous.recording &&
           replaying >= previous.replaying &&
           timerChange >= previous.timerChange;
}

SuiteBridgeBaselineUpdate SuiteBridgeBaselineTracker::apply(
    const SuiteBridgeSnapshotBaseline& next)
{
    if (!hasBaseline_)
    {
        baseline_ = next;
        hasBaseline_ = true;
        deltaAvailable_ = false;
        return SuiteBridgeBaselineUpdate::AdoptedInitial;
    }

    if (next.counterEpoch != baseline_.counterEpoch)
    {
        baseline_ = next;
        deltaAvailable_ = false;
        return SuiteBridgeBaselineUpdate::ReplacedEpochChanged;
    }

    if (next.counterOverflow || baseline_.counterOverflow)
    {
        baseline_ = next;
        deltaAvailable_ = false;
        return SuiteBridgeBaselineUpdate::ReplacedOverflowed;
    }

    if (!next.countersAtLeast(baseline_))
    {
        deltaAvailable_ = false;
        return SuiteBridgeBaselineUpdate::RejectedCounterRegression;
    }

    deltaAvailable_ = next.canCalculateDeltaFrom(baseline_);
    baseline_ = next;
    return SuiteBridgeBaselineUpdate::UpdatedComparable;
}

bool SuiteBridgeBaselineTracker::hasBaseline() const
{
    return hasBaseline_;
}

bool SuiteBridgeBaselineTracker::deltaAvailable() const
{
    return deltaAvailable_;
}

const SuiteBridgeSnapshotBaseline& SuiteBridgeBaselineTracker::baseline() const
{
    if (!hasBaseline_)
    {
        throw std::logic_error("suite bridge baseline is unavailable");
    }

    return baseline_;
}

const char* suiteBridgeHandshakeStatusName(
    const SuiteBridgeHandshakeStatus status)
{
    switch (status)
    {
        case SuiteBridgeHandshakeStatus::Compatible:
            return "compatible";
        case SuiteBridgeHandshakeStatus::Ready:
            return "ready";
        case SuiteBridgeHandshakeStatus::NotConfigured:
            return "not_configured";
        case SuiteBridgeHandshakeStatus::PluginMissing:
            return "plugin_missing";
        case SuiteBridgeHandshakeStatus::LegacyOrUnknown:
            return "legacy_or_unknown";
        case SuiteBridgeHandshakeStatus::DiscoveryTransportError:
            return "discovery_transport_error";
        case SuiteBridgeHandshakeStatus::DiscoveryReplyRejected:
            return "discovery_reply_rejected";
        case SuiteBridgeHandshakeStatus::DiscoveryPayloadTooLarge:
            return "discovery_payload_too_large";
        case SuiteBridgeHandshakeStatus::InvalidDiscoveryPayload:
            return "invalid_discovery_payload";
        case SuiteBridgeHandshakeStatus::UnexpectedPlugin:
            return "unexpected_plugin";
        case SuiteBridgeHandshakeStatus::IncompatibleDiscoverySchema:
            return "incompatible_discovery_schema";
        case SuiteBridgeHandshakeStatus::IncompatibleCapabilitySchema:
            return "incompatible_capability_schema";
        case SuiteBridgeHandshakeStatus::IncompatibleSnapshotSchema:
            return "incompatible_snapshot_schema";
        case SuiteBridgeHandshakeStatus::IncompatibleLocalContractSchema:
            return "incompatible_local_contract_schema";
        case SuiteBridgeHandshakeStatus::RequiredCapabilityUnavailable:
            return "required_capability_unavailable";
        case SuiteBridgeHandshakeStatus::SnapshotTransportError:
            return "snapshot_transport_error";
        case SuiteBridgeHandshakeStatus::SnapshotReplyRejected:
            return "snapshot_reply_rejected";
        case SuiteBridgeHandshakeStatus::SnapshotPayloadTooLarge:
            return "snapshot_payload_too_large";
        case SuiteBridgeHandshakeStatus::InvalidSnapshotPayload:
            return "invalid_snapshot_payload";
        case SuiteBridgeHandshakeStatus::SnapshotInactive:
            return "snapshot_inactive";
    }

    return "unknown";
}

}