#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentRecordingMarksModifyExecutor.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentRecordingMarksModifyCommandContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool backendAgentRecordingMarksModifyCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentRecordingMarksModifyCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentRecordingMarksModifyCommandContext& context,
    IBackendAgentRecordingMarksModifyTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode);

}
