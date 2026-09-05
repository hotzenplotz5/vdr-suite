#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentLocalProvider.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

constexpr const char* kBackendAgentRecordingMarksModifyCommandType =
    "vdr.recording.marks.modify";
constexpr const char* kBackendAgentRecordingMarksModifyAuthorityDomain =
    "vdr.recording.marks";
constexpr const char* kBackendAgentRecordingMarksModifyCapability =
    "vdr.recording.marks.modify";
constexpr const char* kBackendAgentRecordingMarksModifyProviderId =
    "suitebridge:local";
constexpr const char* kBackendAgentRecordingMarksModifyProviderKind =
    "suitebridge";
constexpr std::uint64_t kBackendAgentRecordingMarksModifyPayloadVersion = 1;
constexpr std::size_t kBackendAgentRecordingMarksMaximumReplacementFrames = 256;

enum class BackendAgentRecordingMarksModifyKind
{
    add,
    deleteMark,
    move,
    reset,
    replace,
};

struct BackendAgentRecordingMarksModifyCommand
{
    BackendAgentRecordingMarksModifyKind kind =
        BackendAgentRecordingMarksModifyKind::add;
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    int sourceFrame = -1;
    int targetFrame = -1;
    std::vector<int> replacementFrames;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentLocalProviderSelection localProviderSelection;
};

enum class BackendAgentRecordingMarksModifyOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentRecordingMarksModifyEvidence
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string providerInstanceEpoch;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentRecordingMarksModifyOutcomeCategory outcome =
        BackendAgentRecordingMarksModifyOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

const char* backendAgentRecordingMarksModifyKindName(
    BackendAgentRecordingMarksModifyKind kind);

bool backendAgentRecordingMarksModifyFrameShapeValid(
    BackendAgentRecordingMarksModifyKind kind,
    int sourceFrame,
    int targetFrame,
    const std::vector<int>& replacementFrames);

bool backendAgentRecordingMarksModifyRevisionTokenValid(
    const std::string& value);

bool backendAgentRecordingMarksModifyValidCommand(
    const BackendAgentRecordingMarksModifyCommand& command,
    std::string& reasonCode);

bool backendAgentRecordingMarksModifyEvidenceMatches(
    const BackendAgentRecordingMarksModifyEvidence& evidence,
    const BackendAgentRecordingMarksModifyCommand& command,
    std::string& reasonCode);

}
