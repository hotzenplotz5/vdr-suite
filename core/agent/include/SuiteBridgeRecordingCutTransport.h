#pragma once

#include "BackendAgentRecordingCutTransport.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{

class SuiteBridgeRecordingCutTransport final :
    public IBackendAgentRecordingCutTransport
{
public:
    explicit SuiteBridgeRecordingCutTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;

    BackendAgentRecordingCutTransportReply startCut(
        const BackendAgentRecordingCutTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

}
