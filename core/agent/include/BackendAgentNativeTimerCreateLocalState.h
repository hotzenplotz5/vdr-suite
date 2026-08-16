#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerCreate.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentNativeTimerCreateLocalPhase
{
    starting,
    completed,
};

enum class BackendAgentNativeTimerCreateRecoveryDecision
{
    failClosed,
    reconcileOnly,
    returnPersistedEvidence,
};

struct BackendAgentNativeTimerCreateLocalState
{
    std::uint64_t schemaVersion = 1;
    BackendAgentNativeTimerCreateLocalPhase phase =
        BackendAgentNativeTimerCreateLocalPhase::starting;
    BackendAgentNativeTimerCreateCommand command;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentNativeTimerCreateEvidence evidence;
};

bool backendAgentNativeTimerCreateCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode);

bool backendAgentNativeTimerCreatePrepareLocalStarting(
    const BackendAgentNativeTimerCreateCommand& command,
    std::int64_t persistedAt,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateLocalStateValid(
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateCompleteLocalState(
    BackendAgentNativeTimerCreateLocalState& state,
    const BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode);

BackendAgentNativeTimerCreateRecoveryDecision
backendAgentNativeTimerCreateRecoverLocalState(
    const BackendAgentNativeTimerCreateLocalState& state,
    BackendAgentNativeTimerCreateEvidence& evidence,
    std::string& reasonCode);

std::string backendAgentNativeTimerCreateSerializeLocalState(
    const BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerCreateParseLocalState(
    const std::string& encoded,
    BackendAgentNativeTimerCreateLocalState& state,
    std::string& reasonCode);

} // namespace vdrsuite::agent
