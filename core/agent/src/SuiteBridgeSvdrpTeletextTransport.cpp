#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{

bool safeTeletextChannelId(const std::string& value)
{
    return !value.empty() && value.size() < 64 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

SuiteBridgeTeletextCommandReply toTeletextReply(
    const SuiteBridgeCommandReply& reply)
{
    SuiteBridgeTeletextCommandReply result;
    result.replyCode = reply.replyCode;
    result.payload = reply.payload;
    switch (reply.transportStatus)
    {
        case SuiteBridgeTransportStatus::Success:
            result.transportStatus =
                SuiteBridgeTeletextTransportStatus::Success;
            break;
        case SuiteBridgeTransportStatus::Unavailable:
            result.transportStatus =
                SuiteBridgeTeletextTransportStatus::Unavailable;
            break;
        case SuiteBridgeTransportStatus::Timeout:
            result.transportStatus =
                SuiteBridgeTeletextTransportStatus::Timeout;
            break;
        case SuiteBridgeTransportStatus::Failed:
            result.transportStatus =
                SuiteBridgeTeletextTransportStatus::Failed;
            break;
    }
    result.transportSucceeded =
        result.transportStatus ==
            SuiteBridgeTeletextTransportStatus::Success &&
        reply.replyCode == 250;
    return result;
}

}

SuiteBridgeTeletextCommandReply SuiteBridgeSvdrpTransport::discoverTeletext()
{
    return toTeletextReply(executeRequest(
        "PLUG suitebridge TTXC 1\r\n"));
}

SuiteBridgeTeletextCommandReply SuiteBridgeSvdrpTransport::requestTeletextPage(
    const SuiteBridgeTeletextPageRequest& request)
{
    SuiteBridgeTeletextCommandReply rejected;
    if (!safeTeletextChannelId(request.channelId) ||
        request.pageNumber < 100 || request.pageNumber > 899 ||
        (!request.automaticSubpage && request.subpageCode == 0xffffU))
    {
        return rejected;
    }

    std::ostringstream wire;
    wire << "PLUG suitebridge TTXP 1 "
         << request.channelId << ' '
         << request.pageNumber << ' ';
    if (request.automaticSubpage)
    {
        wire << "auto";
    }
    else
    {
        wire << request.subpageCode;
    }
    wire << "\r\n";

    return toTeletextReply(executeRequest(wire.str()));
}

} // namespace vdrsuite::agent
