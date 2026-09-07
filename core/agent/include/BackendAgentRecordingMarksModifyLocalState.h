#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentRecordingMarksModify.h"

namespace vdrsuite::agent
{

enum class BackendAgentRecordingMarksModifyLocalPhase
{
    starting,
    completed,
};

enum class BackendAgentRecordingMarksModifyRecoveryDecision
{
    failClosed,
    reconcileOnly,
    returnPersistedEvidence,
};

struct BackendAgentRecordingMarksModifyLocalState
{
    std::uint64_t schemaVersion = 1;
    BackendAgentRecordingMarksModifyLocalPhase phase =
        BackendAgentRecordingMarksModifyLocalPhase::starting;
    BackendAgentRecordingMarksModifyCommand command;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentRecordingMarksModifyEvidence evidence;
};

struct BackendAgentRecordingMarksModifyRecoveryResult
{
    BackendAgentRecordingMarksModifyRecoveryDecision decision =
        BackendAgentRecordingMarksModifyRecoveryDecision::failClosed;
    std::string reasonCode;
    BackendAgentRecordingMarksModifyEvidence evidence;
};

bool backendAgentRecordingMarksModifyCommandFromAssignment(
    const BackendAgentCommandAssignment&,
    BackendAgentRecordingMarksModifyCommand&,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyPrepareLocalStarting(
    const BackendAgentCommandAssignment&,
    std::int64_t now,
    BackendAgentRecordingMarksModifyLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyLocalStateValid(
    const BackendAgentRecordingMarksModifyLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyCompleteLocalState(
    BackendAgentRecordingMarksModifyLocalState&,
    const BackendAgentRecordingMarksModifyEvidence&,
    std::string& reasonCode);

BackendAgentRecordingMarksModifyRecoveryResult
backendAgentRecordingMarksModifyRecoverLocalState(
    const BackendAgentRecordingMarksModifyLocalState&,
    const std::string& backendId,
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::int64_t now);

std::string backendAgentRecordingMarksModifySerializeLocalState(
    const BackendAgentRecordingMarksModifyLocalState&,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyParseLocalState(
    const std::string& encoded,
    BackendAgentRecordingMarksModifyLocalState&,
    std::string& reasonCode);

}
