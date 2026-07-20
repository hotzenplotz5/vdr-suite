#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>

namespace vdrsuite::agent
{
namespace
{
bool safeMetadataToken(const std::string& value)
{
    if (value.empty() || value.size() > 255)
    {
        return false;
    }

    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return character > 0x20 && character != 0x7f;
    });
}
}

SuiteBridgeEpgMetadataCommandReply
SuiteBridgeSvdrpTransport::requestEpgMetadata(
    const std::string& channelId,
    const std::string& eventId)
{
    SuiteBridgeEpgMetadataCommandReply metadataReply;
    if (!safeMetadataToken(channelId) || !safeMetadataToken(eventId))
    {
        return metadataReply;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge EPMD " + channelId + " " + eventId + "\r\n");
    metadataReply.transportSucceeded = reply.transportSucceeded();
    metadataReply.replyCode = reply.replyCode;
    metadataReply.payload = reply.payload;
    return metadataReply;
}

}
