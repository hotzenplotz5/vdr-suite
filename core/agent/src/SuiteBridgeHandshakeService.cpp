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

SuiteBridgeHandshakeResult validateDiscovery(
    SuiteBridgeDiscovery discovery)
{
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

    SuiteBridgeHandshakeResult result;
    result.status = SuiteBridgeHandshakeStatus::Compatible;
    result.discovery = std::move(discovery);
    result.mutationsEnabled = false;
    return result;
}

}

SuiteBridgeHandshakeService::SuiteBridgeHandshakeService(
    ISuiteBridgeLocalTransport& transport)
    : transport_(transport)
{
}

SuiteBridgeHandshakeResult SuiteBridgeHandshakeService::discover()
{
    const SuiteBridgeCommandReply discoveryReply =
        transport_.execute(SuiteBridgeLocalCommand::DiscoverSchema1);

    if (!discoveryReply.transportSucceeded())
    {
        if (discoveryReply.transportStatus ==
            SuiteBridgeTransportStatus::Unavailable)
        {
            return failure(
                SuiteBridgeHandshakeStatus::NotConfigured,
                discoveryReply.diagnostic.empty()
                    ? "suite bridge transport is not configured"
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
            SuiteBridgeHandshakeStatus::PluginMissing,
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

    return validateDiscovery(std::move(discoveryParse.value));
}

SuiteBridgeHandshakeResult SuiteBridgeHandshakeService::readSnapshot(
    const SuiteBridgeDiscovery& discovery)
{
    SuiteBridgeHandshakeResult validated = validateDiscovery(discovery);

    if (validated.status != SuiteBridgeHandshakeStatus::Compatible)
    {
        return validated;
    }

    const SuiteBridgeCommandReply snapshotReply =
        transport_.execute(SuiteBridgeLocalCommand::Snapshot);

    if (!snapshotReply.transportSucceeded())
    {
        SuiteBridgeHandshakeResult result = failure(
            SuiteBridgeHandshakeStatus::SnapshotTransportError,
            snapshotReply.diagnostic.empty()
                ? "suite bridge snapshot transport failed"
                : snapshotReply.diagnostic);
        result.discovery = discovery;
        return result;
    }

    if (snapshotReply.replyCode != 900)
    {
        SuiteBridgeHandshakeResult result = failure(
            SuiteBridgeHandshakeStatus::SnapshotReplyRejected,
            "suite bridge SNAP reply rejected");
        result.discovery = discovery;
        return result;
    }

    SuiteBridgeSnapshotParseResult snapshotParse =
        parser_.parseSnapshot(snapshotReply.payload);

    if (!snapshotParse.ok())
    {
        SuiteBridgeHandshakeResult result = failure(
            snapshotParseFailureStatus(snapshotParse.status),
            snapshotParse.diagnostic);
        result.discovery = discovery;
        return result;
    }

    SuiteBridgeSnapshotBaseline baseline =
        std::move(snapshotParse.value);

    if (baseline.contractSchema != discovery.localContractSchema ||
        baseline.capabilitySchema != discovery.capabilitySchema ||
        baseline.snapshotSchema != discovery.snapshotSchema)
    {
        SuiteBridgeHandshakeResult result = failure(
            SuiteBridgeHandshakeStatus::InvalidSnapshotPayload,
            "snapshot schemas do not match discovery");
        result.discovery = discovery;
        return result;
    }

    if (!baseline.active)
    {
        SuiteBridgeHandshakeResult result = failure(
            SuiteBridgeHandshakeStatus::SnapshotInactive,
            "suite bridge snapshot is inactive");
        result.discovery = discovery;
        return result;
    }

    SuiteBridgeHandshakeResult result;
    result.status = SuiteBridgeHandshakeStatus::Ready;
    result.discovery = discovery;
    result.baseline = std::move(baseline);
    result.mutationsEnabled = false;
    return result;
}

SuiteBridgeHandshakeResult SuiteBridgeHandshakeService::perform()
{
    SuiteBridgeHandshakeResult discovery = discover();

    if (discovery.status == SuiteBridgeHandshakeStatus::NotConfigured ||
        discovery.status == SuiteBridgeHandshakeStatus::PluginMissing)
    {
        discovery.status = SuiteBridgeHandshakeStatus::LegacyOrUnknown;
        return discovery;
    }

    if (discovery.status != SuiteBridgeHandshakeStatus::Compatible)
    {
        return discovery;
    }

    return readSnapshot(discovery.discovery);
}

}