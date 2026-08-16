#pragma once

#include "BackendAgentNativeTimerCreate.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

inline constexpr const char* kBackendAgentNativeTimerUpdateCommandType = "vdr.timer.update";
inline constexpr const char* kBackendAgentNativeTimerToggleCommandType = "vdr.timer.toggle";
inline constexpr const char* kBackendAgentNativeTimerModifyAuthorityDomain = "vdr.timer";
inline constexpr const char* kBackendAgentNativeTimerUpdateCapability = "vdr.timer.update";
inline constexpr const char* kBackendAgentNativeTimerToggleCapability = "vdr.timer.toggle";
inline constexpr const char* kBackendAgentNativeTimerModifyProviderId = "suitebridge:local";
inline constexpr const char* kBackendAgentNativeTimerModifyProviderKind = "suitebridge";
inline constexpr std::uint64_t kBackendAgentNativeTimerModifyPayloadVersion = 1;

enum class BackendAgentNativeTimerModifyKind { update, toggle };
const char* backendAgentNativeTimerModifyKindName(BackendAgentNativeTimerModifyKind kind);
const char* backendAgentNativeTimerModifyCapability(BackendAgentNativeTimerModifyKind kind);

struct BackendAgentNativeTimerModifyCommand
{
    BackendAgentNativeTimerModifyKind kind = BackendAgentNativeTimerModifyKind::update;
    std::string commandId;
    std::string requestFingerprint;
    std::string operationId;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string backendNativeTimerId;
    std::string expectedCurrentFingerprint;
    std::string expectedSpecificationFingerprint;
    BackendAgentNativeTimerCreateSpecification specification;
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

enum class BackendAgentNativeTimerModifyOutcomeCategory
{
    rejectedWithoutEffect,
    acceptedUnverified,
    outcomeUnknown,
};

struct BackendAgentNativeTimerModifyEvidence
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
    BackendAgentNativeTimerModifyOutcomeCategory outcome =
        BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown;
    std::int64_t dispatchStartedAt = 0;
    std::int64_t completedAt = 0;
    std::string evidenceReference;
};

bool backendAgentNativeTimerModifyValidCommand(
    const BackendAgentNativeTimerModifyCommand& command,
    std::string& reasonCode);
bool backendAgentNativeTimerModifyEvidenceMatches(
    const BackendAgentNativeTimerModifyEvidence& evidence,
    const BackendAgentNativeTimerModifyCommand& command,
    std::string& reasonCode);

}
