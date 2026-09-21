#pragma once

#include "BackendAgentLocalProvider.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

constexpr const char* kBackendAgentNativeTimerCreateCommandType = "vdr.timer.create";
constexpr const char* kBackendAgentNativeTimerCreateAuthorityDomain = "vdr.timer";
constexpr const char* kBackendAgentNativeTimerCreateCapability = "vdr.timer.create";
constexpr const char* kBackendAgentNativeTimerCreateProviderId = "suitebridge:local";
constexpr const char* kBackendAgentNativeTimerCreateProviderKind = "suitebridge";
constexpr std::uint64_t kBackendAgentNativeTimerCreatePayloadVersion = 1;

struct BackendAgentNativeTimerCreateSpecification
{
    std::string channelId;
    std::string title;
    std::string directory;
    std::string day;
    std::string weekdays = "-------";
    std::string startTime;
    std::string endTime;
    std::int32_t priority = 50;
    std::int32_t lifetime = 99;
    bool enabled = true;
    bool vps = false;
};

struct BackendAgentNativeTimerCreateCommand
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string expectedSpecificationFingerprint;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentNativeTimerCreateSpecification specification;
    BackendAgentLocalProviderSelection localProviderSelection;
};

enum class BackendAgentNativeTimerCreateOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentNativeTimerCreateEvidence
{
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string nativeTimerBindingId;
    std::string jobId;
    std::string attemptId;
    std::uint64_t claimEpoch = 0;
    std::string backendId;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string providerInstanceEpoch;
    std::int64_t localStartingPersistedAt = 0;
    BackendAgentNativeTimerCreateOutcomeCategory outcome =
        BackendAgentNativeTimerCreateOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

bool backendAgentNativeTimerCreateSpecificationValid(
    const BackendAgentNativeTimerCreateSpecification& specification);
std::string backendAgentNativeTimerCreateSpecificationFingerprint(
    const BackendAgentNativeTimerCreateSpecification& specification);
bool backendAgentNativeTimerCreateValidCommand(
    const BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode);
bool backendAgentNativeTimerCreateEvidenceMatches(
    const BackendAgentNativeTimerCreateEvidence& evidence,
    const BackendAgentNativeTimerCreateCommand& command,
    std::string& reasonCode);

} // namespace vdrsuite::agent
