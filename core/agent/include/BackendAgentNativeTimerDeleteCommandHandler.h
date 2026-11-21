#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeTimerDeleteExecutor.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerDeleteCommandContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool backendAgentNativeTimerDeleteCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerDeleteCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerDeleteCommandContext& context,
    IBackendAgentNativeTimerDeleteTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode);

}
