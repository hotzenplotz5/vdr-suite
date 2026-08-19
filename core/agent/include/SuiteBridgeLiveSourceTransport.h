#pragma once

#include "ISuiteBridgeLocalTransport.h"

#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeLiveSourceOpenRequest
{
    std::string leaseId;
    std::string channelId;
    std::string pluginInstanceEpoch;
};

struct SuiteBridgeLiveSourceLeaseRequest
{
    std::string leaseId;
    std::string pluginInstanceEpoch;
};

class ISuiteBridgeLiveSourceTransport
{
public:
    virtual ~ISuiteBridgeLiveSourceTransport() = default;

    virtual SuiteBridgeCommandReply discoverLiveSource() = 0;
    virtual SuiteBridgeCommandReply openLiveSource(
        const SuiteBridgeLiveSourceOpenRequest& request) = 0;
    virtual SuiteBridgeCommandReply closeLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest& request) = 0;
    virtual SuiteBridgeCommandReply statusLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest& request) = 0;
};

}
