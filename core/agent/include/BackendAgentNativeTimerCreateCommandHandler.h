#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentNativeTimerCreateExecutor.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerCreateCommandContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool backendAgentNativeTimerCreateCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentNativeTimerCreateCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentNativeTimerCreateCommandContext& context,
    IBackendAgentNativeTimerCreateTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode);

} // namespace vdrsuite::agent
