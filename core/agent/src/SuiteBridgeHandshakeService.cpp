#include "SuiteBridgeHandshakeService.h"

#include <utility>

namespace vdrsuite::agent
{
namespace
{

SuiteBridgeHandshakeResult failure(
    const SuiteBridgeHandshakeStatus status,
    std::string diagnostic)
{
    SuiteBridgeHandshakeResult result;
    result.status = status;
    result.diagnostic = std::move(diagnostic);
    result.mutationsEnabled = false;
    return result;
}

SuiteBridgeHandshakeStatus discoveryParseFailureStatus(
    const SuiteBridgeParseStatus status)
{
    if (status == SuiteBridgeParseStatus::PayloadTooLarge)
    {
        return SuiteBridgeHandshakeStatus::DiscoveryPayloadTooLarge;
    }

    return SuiteBridgeHandshakeStatus::InvalidDiscoveryPayload;
}

SuiteBridgeHandshakeStatus snapshotParseFailureStatus(
    const SuiteBridgeParseStatus status)
{
    if (status == SuiteBridgeParseStatus::PayloadTooLarge)
    {
        return SuiteBridgeHandshakeStatus::SnapshotPayloadTooLarge;
    }

    return SuiteBridgeHandshakeStatus::InvalidSnapshotPayload;
}

}

SuiteBridgeHandshakeService::SuiteBridgeHandshakeService(
    ISuiteBridgeLocalTransport& transport)
    : transport_(transport)
{
}

SuiteBridgeHandshakeResult SuiteBridgeHandshakeService::perform()
{
    const SuiteBridgeCommandReply discoveryReply =
        transport_.execute(SuiteBridgeLocalCommand::DiscoverSchema1);

    if (!discoveryReply.transportSucceeded())
    {
        if (discoveryReply.transportStatus ==
            SuiteBridgeTransportStatus::Unavailable)
        {
            return failure(
                SuiteBridgeHandshakeStatus::LegacyOrUnknown,
                discoveryReply.diagnostic.empty()
                    ? "suite bridge discovery unavailable"
                    : discoveryReply.diagnostic);
        }

        return failure(
            SuiteBridgeHandshakeStatus::DiscoveryTransportError,
            discoveryReply.diagnostic.empty()
                ? "suite bridge discovery transport failed"
                : discoveryReply.diagnostic);
    }

    if (discoveryReply.replyCode == 500)
    {
        return failure(
            SuiteBridgeHandshakeStatus::LegacyOrUnknown,
            "suite bridge CAPS command unavailable");
    }

    if (discoveryReply.replyCode == 550)
    {
        return failure(
            SuiteBridgeHandshakeStatus::LegacyOrUnknown,
            "suite bridge plugin unavailable");
    }

    if (discoveryReply.replyCode != 900)
    {
        return failure(
            SuiteBridgeHandshakeStatus::DiscoveryReplyRejected,
            "suite bridge CAPS reply rejected");
    }

    SuiteBridgeDiscoveryParseResult discoveryParse =
        parser_.parseDiscovery(discoveryReply.payload);

    if (!discoveryParse.ok())
    {
        return failure(
            discoveryParseFailureStatus(discoveryParse.status),
            discoveryParse.diagnostic);
    }

    SuiteBridgeDiscovery discovery =
        std::move(discoveryParse.value);

    if (discovery.pluginName != "suitebridge")
    {
        return failure(
            SuiteBridgeHandshakeStatus::UnexpectedPlugin,
            "unexpected local plugin identity");
    }

    if (discovery.discoverySchema != 1)
    {
        return failure(
            SuiteBridgeHandshakeStatus::IncompatibleDiscoverySchema,
            "unsupported suite bridge discovery schema");
    }

    if (discovery.capabilitySchema != 1)
    {
        return failure(
            SuiteBridgeHandshakeStatus::IncompatibleCapabilitySchema,
            "unsupported suite bridge capability schema");
    }

    if (discovery.snapshotSchema != 2)
    {
        return failure(
            SuiteBridgeHandshakeStatus::IncompatibleSnapshotSchema,
            "unsupported suite bridge snapshot schema");
    }

    if (discovery.localContractSchema != 2)
    {
        return failure(
            SuiteBridgeHandshakeStatus::IncompatibleLocalContractSchema,
            "unsupported suite bridge local contract schema");
    }

    if (!discovery.capabilityAvailable("snapshots") ||
        !discovery.capabilityAvailable("local-contract"))
    {
        return failure(
            SuiteBridgeHandshakeStatus::RequiredCapabilityUnavailable,
            "required suite bridge read-only capability unavailable");
    }

    const SuiteBridgeCommandReply snapshotReply =
        transport_.execute(SuiteBridgeLocalCommand::Snapshot);

    if (!snapshotReply.transportSucceeded())
    {
        return failure(
            SuiteBridgeHandshakeStatus::SnapshotTransportError,
            snapshotReply.diagnostic.empty()
                ? "suite bridge snapshot transport failed"
                : snapshotReply.diagnostic);
    }

    if (snapshotReply.replyCode != 900)
    {
        return failure(
            SuiteBridgeHandshakeStatus::SnapshotReplyRejected,
            "suite bridge SNAP reply rejected");
    }

    SuiteBridgeSnapshotParseResult snapshotParse =
        parser_.parseSnapshot(snapshotReply.payload);

    if (!snapshotParse.ok())
    {
        return failure(
            snapshotParseFailureStatus(snapshotParse.status),
            snapshotParse.diagnostic);
    }

    SuiteBridgeSnapshotBaseline baseline =
        std::move(snapshotParse.value);

    if (baseline.contractSchema != discovery.localContractSchema ||
        baseline.capabilitySchema != discovery.capabilitySchema ||
        baseline.snapshotSchema != discovery.snapshotSchema)
    {
        return failure(
            SuiteBridgeHandshakeStatus::InvalidSnapshotPayload,
            "snapshot schemas do not match discovery");
    }

    if (!baseline.active)
    {
        return failure(
            SuiteBridgeHandshakeStatus::SnapshotInactive,
            "suite bridge snapshot is inactive");
    }

    SuiteBridgeHandshakeResult result;
    result.status = SuiteBridgeHandshakeStatus::Ready;
    result.discovery = std::move(discovery);
    result.baseline = std::move(baseline);

    // SB.10a is intentionally read-only even if a future plugin reports
    // a mutation capability as available. Plugin capability is not Agent
    // policy and is never sufficient authorization.
    result.mutationsEnabled = false;
    return result;
}

}