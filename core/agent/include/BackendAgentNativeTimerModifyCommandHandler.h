#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeTimerModifyExecutor.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerModifyCommandContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool backendAgentNativeTimerModifyCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerModifyCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerModifyCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode);

bool backendAgentNativeTimerModifyCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerModifyCommandContext& context,
    IBackendAgentNativeTimerModifyTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode);

}
