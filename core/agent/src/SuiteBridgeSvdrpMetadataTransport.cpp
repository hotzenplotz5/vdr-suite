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

    return std::all_of(value.begin(), value.end(), [](unsigned char ch) {
        return ch > 0x20 && ch != 0x7f;
    });
}

}

SuiteBridgeMetadataCommandReply SuiteBridgeSvdrpTransport::requestMetadata(
    const std::string& channelId,
    const std::string& eventId)
{
    SuiteBridgeMetadataCommandReply metadataReply;
    if (!safeMetadataToken(channelId) || !safeMetadataToken(eventId))
    {
        return metadataReply;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge META " + channelId + " " + eventId + "\r\n");
    metadataReply.replyCode = reply.replyCode;
    metadataReply.payload = reply.payload;
    switch (reply.transportStatus)
    {
        case SuiteBridgeTransportStatus::Success:
            metadataReply.transportStatus = SuiteBridgeReadTransportStatus::Success;
            break;
        case SuiteBridgeTransportStatus::Unavailable:
            metadataReply.transportStatus = SuiteBridgeReadTransportStatus::Unavailable;
            break;
        case SuiteBridgeTransportStatus::Timeout:
            metadataReply.transportStatus = SuiteBridgeReadTransportStatus::Timeout;
            break;
        case SuiteBridgeTransportStatus::Failed:
            metadataReply.transportStatus = SuiteBridgeReadTransportStatus::Failed;
            break;
    }
    metadataReply.transportSucceeded =
        metadataReply.transportStatus == SuiteBridgeReadTransportStatus::Success &&
        reply.replyCode == 250;
    return metadataReply;
}

}
