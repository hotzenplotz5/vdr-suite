#pragma once

#include "BackendAgentRecordingCutLocalState.h"
#include "BackendAgentRecordingCutTransport.h"

namespace vdrsuite::agent
{

struct BackendAgentRecordingCutExecutorContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t now = 0;
};

bool backendAgentRecordingCutExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment&,
    const BackendAgentRecordingCutLocalState&,
    const BackendAgentRecordingCutExecutorContext&,
    IBackendAgentRecordingCutTransport&,
    BackendAgentRecordingCutEvidence&,
    std::string& reasonCode);

}
