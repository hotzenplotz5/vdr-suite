#pragma once

#include "BackendAgentNativeTimerCreateExecutor.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{

class SuiteBridgeNativeTimerCreateTransport final :
    public IBackendAgentNativeTimerCreateTransport
{
public:
    explicit SuiteBridgeNativeTimerCreateTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;

    BackendAgentNativeTimerCreateTransportReply createTimer(
        const BackendAgentNativeTimerCreateTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

} // namespace vdrsuite::agent
