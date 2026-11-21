#include "SuiteBridgeObservation.h"

namespace vdrsuite::agent
{

const char* suiteBridgeObservationStateName(
    const SuiteBridgeObservationState state)
{
    switch (state)
    {
        case SuiteBridgeObservationState::NotConfigured:
            return "not_configured";
        case SuiteBridgeObservationState::Connecting:
            return "connecting";
        case SuiteBridgeObservationState::PluginMissing:
            return "plugin_missing";
        case SuiteBridgeObservationState::LegacyOrUnknown:
            return "legacy_or_unknown";
        case SuiteBridgeObservationState::Incompatible:
            return "incompatible";
        case SuiteBridgeObservationState::Compatible:
            return "compatible";
        case SuiteBridgeObservationState::SnapshotCurrent:
            return "snapshot_current";
        case SuiteBridgeObservationState::SnapshotStale:
            return "snapshot_stale";
        case SuiteBridgeObservationState::TransportDegraded:
            return "transport_degraded";
        case SuiteBridgeObservationState::Overflowed:
            return "overflowed";
        case SuiteBridgeObservationState::Offline:
            return "offline";
    }

    return "unknown";
}

}