#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace vdrsuite::agent
{
namespace
{

bool safeHbbtvToken(const std::string& value, std::size_t maximumLength)
{
    return !value.empty() && value.size() < maximumLength &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

const char* runtimeOperationName(SuiteBridgeHbbtvRuntimeOperation operation)
{
    switch (operation)
    {
        case SuiteBridgeHbbtvRuntimeOperation::Launch: return "LAUNCH";
        case SuiteBridgeHbbtvRuntimeOperation::Status: return "STATUS";
        case SuiteBridgeHbbtvRuntimeOperation::Input: return "INPUT";
        case SuiteBridgeHbbtvRuntimeOperation::Close: return "CLOSE";
    }
    return nullptr;
}

const char* inputActionName(SuiteBridgeHbbtvInputAction action)
{
    switch (action)
    {
        case SuiteBridgeHbbtvInputAction::None: return nullptr;
        case SuiteBridgeHbbtvInputAction::Up: return "UP";
        case SuiteBridgeHbbtvInputAction::Down: return "DOWN";
        case SuiteBridgeHbbtvInputAction::Left: return "LEFT";
        case SuiteBridgeHbbtvInputAction::Right: return "RIGHT";
        case SuiteBridgeHbbtvInputAction::Ok: return "OK";
        case SuiteBridgeHbbtvInputAction::Back: return "BACK";
        case SuiteBridgeHbbtvInputAction::Red: return "RED";
        case SuiteBridgeHbbtvInputAction::Green: return "GREEN";
        case SuiteBridgeHbbtvInputAction::Yellow: return "YELLOW";
        case SuiteBridgeHbbtvInputAction::Blue: return "BLUE";
        case SuiteBridgeHbbtvInputAction::Digit0: return "0";
        case SuiteBridgeHbbtvInputAction::Digit1: return "1";
        case SuiteBridgeHbbtvInputAction::Digit2: return "2";
        case SuiteBridgeHbbtvInputAction::Digit3: return "3";
        case SuiteBridgeHbbtvInputAction::Digit4: return "4";
        case SuiteBridgeHbbtvInputAction::Digit5: return "5";
        case SuiteBridgeHbbtvInputAction::Digit6: return "6";
        case SuiteBridgeHbbtvInputAction::Digit7: return "7";
        case SuiteBridgeHbbtvInputAction::Digit8: return "8";
        case SuiteBridgeHbbtvInputAction::Digit9: return "9";
        case SuiteBridgeHbbtvInputAction::Play: return "PLAY";
        case SuiteBridgeHbbtvInputAction::Pause: return "PAUSE";
        case SuiteBridgeHbbtvInputAction::Stop: return "STOP";
        case SuiteBridgeHbbtvInputAction::FastForward: return "FAST_FORWARD";
        case SuiteBridgeHbbtvInputAction::Rewind: return "REWIND";
    }
    return nullptr;
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
    if (!safeHbbtvToken(channelId, 64))
    {
        return rejected;
    }

    std::ostringstream wire;
    wire << "PLUG suitebridge HBBAPPS 1 " << channelId << "\r\n";
    return toHbbtvReply(executeRequest(wire.str()));
}

SuiteBridgeHbbtvCommandReply SuiteBridgeSvdrpTransport::controlHbbtv(
    const SuiteBridgeHbbtvRuntimeRequest& request)
{
    SuiteBridgeHbbtvCommandReply rejected;

    const char* operation = runtimeOperationName(request.operation);
    if (operation == nullptr ||
        !safeHbbtvToken(request.sessionId, 128) ||
        !safeHbbtvToken(request.channelId, 64) ||
        request.applicationId == 0 ||
        request.descriptorRevision == 0)
    {
        return rejected;
    }

    const bool isInput =
        request.operation == SuiteBridgeHbbtvRuntimeOperation::Input;
    const char* action = inputActionName(request.inputAction);
    if ((isInput && action == nullptr) ||
        (!isInput && request.inputAction != SuiteBridgeHbbtvInputAction::None))
    {
        return rejected;
    }

    std::ostringstream wire;
    wire << "PLUG suitebridge HBBRUN " << operation << " 1 "
         << request.sessionId << ' '
         << request.channelId << ' '
         << request.applicationId << ' '
         << request.descriptorRevision;

    if (isInput)
        wire << ' ' << action;

    wire << "\r\n";
    return toHbbtvReply(executeRequest(wire.str()));
}

} // namespace vdrsuite::agent
