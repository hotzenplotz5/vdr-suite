#pragma once

#include "BackendAgentLocalProvider.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

constexpr const char* kBackendAgentNativeTimerDeleteCommandType = "vdr.timer.delete";
constexpr const char* kBackendAgentNativeTimerDeleteAuthorityDomain = "vdr.timer";
constexpr const char* kBackendAgentNativeTimerDeleteCapability = "vdr.timer.delete";
constexpr const char* kBackendAgentNativeTimerDeleteProviderId = "suitebridge:local";
constexpr const char* kBackendAgentNativeTimerDeleteProviderKind = "suitebridge";
constexpr std::uint64_t kBackendAgentNativeTimerDeletePayloadVersion = 1;

struct BackendAgentNativeTimerDeleteCommand
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string expectedNativeTimerFingerprint;
    std::string timerAssignmentId;
    std::string backendNativeTimerId;
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

enum class BackendAgentNativeTimerDeleteOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentNativeTimerDeleteEvidence
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
    BackendAgentNativeTimerDeleteOutcomeCategory outcome =
        BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

bool backendAgentNativeTimerDeleteValidCommand(
    const BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode);

bool backendAgentNativeTimerDeleteEvidenceMatches(
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    const BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode);

}
