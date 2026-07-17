#pragma once

#include <string>

namespace vdrsuite::agent
{

enum class SuiteBridgeLocalCommand
{
    DiscoverSchema1,
    Snapshot
};

enum class SuiteBridgeTransportStatus
{
    Success,
    Unavailable,
    Timeout,
    Failed
};

struct SuiteBridgeCommandReply
{
    SuiteBridgeTransportStatus transportStatus =
        SuiteBridgeTransportStatus::Failed;
    int replyCode = 0;
    std::string payload;
    std::string diagnostic;

    bool transportSucceeded() const
    {
        return transportStatus == SuiteBridgeTransportStatus::Success;
    }
};

class ISuiteBridgeLocalTransport
{
public:
    virtual ~ISuiteBridgeLocalTransport() = default;

    virtual SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) = 0;
};

}
