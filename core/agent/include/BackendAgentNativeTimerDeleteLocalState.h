#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentNativeTimerDelete.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

enum class BackendAgentNativeTimerDeleteLocalPhase
{
    starting,
    completed,
};

enum class BackendAgentNativeTimerDeleteRecoveryDecision
{
    failClosed,
    reconcileOnly,
    returnPersistedEvidence,
};

struct BackendAgentNativeTimerDeleteLocalState
{
    std::uint64_t schemaVersion = 1;
    BackendAgentNativeTimerDeleteLocalPhase phase =
        BackendAgentNativeTimerDeleteLocalPhase::starting;
    BackendAgentNativeTimerDeleteCommand command;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentNativeTimerDeleteEvidence evidence;
};

struct BackendAgentNativeTimerDeleteRecoveryResult
{
    BackendAgentNativeTimerDeleteRecoveryDecision decision =
        BackendAgentNativeTimerDeleteRecoveryDecision::failClosed;
    std::string reasonCode;
    BackendAgentNativeTimerDeleteEvidence evidence;
};

bool backendAgentNativeTimerDeleteCommandFromAssignment(
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode);

// Canonical durable Agent result evidence. Unlike generic command categories,
// this carries the observed post-dispatch starting boundary needed for safe
// absence readback. No caller may manufacture that boundary from claim time.
std::string backendAgentNativeTimerDeleteResultEvidence(
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    const BackendAgentCommandAssignment& assignment,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteParseResultEvidence(
    const std::string& encoded,
    const BackendAgentCommandAssignment& assignment,
    BackendAgentNativeTimerDeleteEvidence& evidence,
    std::string& reasonCode);

bool backendAgentNativeTimerDeletePrepareLocalStarting(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteLocalStateValid(
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteCompleteLocalState(
    BackendAgentNativeTimerDeleteLocalState& state,
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    std::string& reasonCode);

BackendAgentNativeTimerDeleteRecoveryResult
backendAgentNativeTimerDeleteRecoverLocalState(
    const BackendAgentNativeTimerDeleteLocalState& state,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now);

std::string backendAgentNativeTimerDeleteSerializeLocalState(
    const BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteParseLocalState(
    const std::string& encoded,
    BackendAgentNativeTimerDeleteLocalState& state,
    std::string& reasonCode);

}
