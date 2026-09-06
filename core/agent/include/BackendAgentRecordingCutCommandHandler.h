#pragma once

#include "BackendAgentCommandStateStore.h"
#include "BackendAgentRecordingCutExecutor.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentRecordingCutCommandContext
{
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
};

bool backendAgentRecordingCutCommandReconcileExisting(
    const std::string& statePath,
    const BackendAgentRecordingCutCommandContext& context,
    commandstate::LocalState& state,
    std::string& reasonCode);

bool backendAgentRecordingCutCommandPrepareFreshStarting(
    const std::string& statePath,
    commandstate::LocalState& state,
    std::int64_t currentTime,
    std::string& reasonCode);

bool backendAgentRecordingCutCommandExecuteFreshStartingAndPersistOutcome(
    const std::string& statePath,
    const BackendAgentRecordingCutCommandContext& context,
    IBackendAgentRecordingCutTransport* transport,
    commandstate::LocalState& state,
    std::string& reasonCode);

}
