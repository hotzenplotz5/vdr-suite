#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentRecordingCut.h"

namespace vdrsuite::agent
{

enum class BackendAgentRecordingCutLocalPhase
{
    starting,
    completed,
};

enum class BackendAgentRecordingCutRecoveryDecision
{
    failClosed,
    reconcileOnly,
    returnPersistedEvidence,
};

struct BackendAgentRecordingCutLocalState
{
    std::uint64_t schemaVersion = 1;
    BackendAgentRecordingCutLocalPhase phase =
        BackendAgentRecordingCutLocalPhase::starting;
    BackendAgentRecordingCutCommand command;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentRecordingCutEvidence evidence;
};

struct BackendAgentRecordingCutRecoveryResult
{
    BackendAgentRecordingCutRecoveryDecision decision =
        BackendAgentRecordingCutRecoveryDecision::failClosed;
    std::string reasonCode;
    BackendAgentRecordingCutEvidence evidence;
};

bool backendAgentRecordingCutCommandFromAssignment(
    const BackendAgentCommandAssignment&,
    BackendAgentRecordingCutCommand&,
    std::string& reasonCode);

bool backendAgentRecordingCutPrepareLocalStarting(
    const BackendAgentCommandAssignment&,
    std::int64_t now,
    BackendAgentRecordingCutLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingCutLocalStateValid(
    const BackendAgentRecordingCutLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingCutCompleteLocalState(
    BackendAgentRecordingCutLocalState&,
    const BackendAgentRecordingCutEvidence&,
    std::string& reasonCode);

BackendAgentRecordingCutRecoveryResult backendAgentRecordingCutRecoverLocalState(
    const BackendAgentRecordingCutLocalState&,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now);

std::string backendAgentRecordingCutSerializeLocalState(
    const BackendAgentRecordingCutLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingCutParseLocalState(
    const std::string& encoded,
    BackendAgentRecordingCutLocalState&,
    std::string& reasonCode);

}
