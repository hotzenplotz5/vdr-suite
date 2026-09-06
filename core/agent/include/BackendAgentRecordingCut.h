#pragma once

#include "BackendAgentCommand.h"
#include "BackendAgentLocalProvider.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

constexpr const char* kBackendAgentRecordingCutCommandType =
    "vdr.recording.cut";
constexpr const char* kBackendAgentRecordingCutAuthorityDomain =
    "vdr.recording.cut";
constexpr const char* kBackendAgentRecordingCutCapability =
    "vdr.recording.cut";
constexpr const char* kBackendAgentRecordingCutProviderId =
    "suitebridge:recording-cut";
constexpr const char* kBackendAgentRecordingCutProviderKind =
    "suitebridge";
constexpr std::uint64_t kBackendAgentRecordingCutPayloadVersion = 1;

struct BackendAgentRecordingCutCommand
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
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

enum class BackendAgentRecordingCutOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentRecordingCutEvidence
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
    BackendAgentRecordingCutOutcomeCategory outcome =
        BackendAgentRecordingCutOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

bool backendAgentRecordingCutRevisionTokenValid(const std::string& value);

bool backendAgentRecordingCutValidCommand(
    const BackendAgentRecordingCutCommand& command,
    std::string& reasonCode);

bool backendAgentRecordingCutEvidenceMatches(
    const BackendAgentRecordingCutEvidence& evidence,
    const BackendAgentRecordingCutCommand& command,
    std::string& reasonCode);

}
