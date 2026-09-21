#pragma once

#include "BackendAgentRecordingMarksModifyTransport.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{

class SuiteBridgeRecordingMarksModifyTransport final :
    public IBackendAgentRecordingMarksModifyTransport
{
public:
    explicit SuiteBridgeRecordingMarksModifyTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;

    BackendAgentRecordingMarksModifyTransportReply modifyMarks(
        const BackendAgentRecordingMarksModifyTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

}
