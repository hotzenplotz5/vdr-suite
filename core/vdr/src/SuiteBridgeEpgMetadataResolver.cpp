#include "SuiteBridgeEpgMetadataResolver.h"

#include <chrono>

namespace
{
long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
}

SuiteBridgeEpgMetadataResolver::SuiteBridgeEpgMetadataResolver(
    ISuiteBridgeEpgMetadataTransport& transport)
    : transport_(transport)
{
}

EpgMetadataResolution SuiteBridgeEpgMetadataResolver::resolve(
    const std::string& backendId,
    const VdrEvent& event)
{
    EpgMetadataResolution resolution;
    if (backendId.empty() || event.channelId.empty() || event.id.empty())
    {
        return resolution;
    }

    const SuiteBridgeEpgMetadataCommandReply reply =
        transport_.requestEpgMetadata(event.channelId, event.id);
    if (!reply.transportSucceeded)
    {
        return resolution;
    }

    resolution.attempted = true;
    if (reply.replyCode != 250)
    {
        return resolution;
    }

    if (reply.payload ==
        "{\"schema\":1,\"found\":false,\"provider\":\"none\"}")
    {
        return resolution;
    }

    resolution.metadata = parser_.parse(
        reply.payload,
        backendId,
        event.channelId,
        event.id,
        epochSeconds());

    if (!resolution.metadata.valid())
    {
        resolution.attempted = false;
        return resolution;
    }

    resolution.found = true;
    return resolution;
}
