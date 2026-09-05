#pragma once

#include "BackendAgentRecordingMarksModifyLocalState.h"
#include "BackendAgentRecordingMarksModifyTransport.h"

namespace vdrsuite::agent
{

struct BackendAgentRecordingMarksModifyExecutorContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t now = 0;
};

bool backendAgentRecordingMarksModifyExecuteFreshStartingOnce(
    const BackendAgentCommandAssignment&,
    const BackendAgentRecordingMarksModifyLocalState&,
    const BackendAgentRecordingMarksModifyExecutorContext&,
    IBackendAgentRecordingMarksModifyTransport&,
    BackendAgentRecordingMarksModifyEvidence&,
    std::string& reasonCode);

}
