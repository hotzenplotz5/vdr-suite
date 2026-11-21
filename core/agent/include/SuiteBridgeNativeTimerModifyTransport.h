#pragma once

#include "BackendAgentNativeTimerModifyExecutor.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{
class SuiteBridgeNativeTimerModifyTransport final
    : public IBackendAgentNativeTimerModifyTransport
{
public:
    explicit SuiteBridgeNativeTimerModifyTransport(
        SuiteBridgeSvdrpTransportConfig config = {});
    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;
    BackendAgentNativeTimerModifyTransportReply modifyTimer(
        const BackendAgentNativeTimerModifyTransportRequest& request) override;
private:
    SuiteBridgeSvdrpTransport transport_;
};
}
