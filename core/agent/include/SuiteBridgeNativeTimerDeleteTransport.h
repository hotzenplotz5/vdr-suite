#pragma once

#include "BackendAgentNativeTimerDeleteExecutor.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{

class SuiteBridgeNativeTimerDeleteTransport final :
    public IBackendAgentNativeTimerDeleteTransport
{
public:
    explicit SuiteBridgeNativeTimerDeleteTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;

    BackendAgentNativeTimerDeleteTransportReply deleteTimer(
        const BackendAgentNativeTimerDeleteTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

}
