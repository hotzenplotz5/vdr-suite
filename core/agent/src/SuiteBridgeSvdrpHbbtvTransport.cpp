#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{

bool safeHbbtvChannelId(const std::string& value)
{
    return !value.empty() && value.size() < 64 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

SuiteBridgeHbbtvCommandReply toHbbtvReply(
    const SuiteBridgeCommandReply& reply)
{
    SuiteBridgeHbbtvCommandReply result;
    result.replyCode = reply.replyCode;
    result.payload = reply.payload;
    result.transportSucceeded =
        reply.transportSucceeded() && reply.replyCode == 250;
    return result;
}

}

SuiteBridgeHbbtvCommandReply SuiteBridgeSvdrpTransport::discoverHbbtv(
    const std::string& channelId)
{
    SuiteBridgeHbbtvCommandReply rejected;
    if (!safeHbbtvChannelId(channelId))
    {
        return rejected;
    }

    std::ostringstream wire;
    wire << "PLUG suitebridge HBBAPPS 1 " << channelId << "\r\n";
    return toHbbtvReply(executeRequest(wire.str()));
}

} // namespace vdrsuite::agent
