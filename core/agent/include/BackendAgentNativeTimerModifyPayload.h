#pragma once

#include "BackendAgentNativeTimerModify.h"

namespace vdrsuite::agent
{
struct BackendAgentNativeTimerModifyPayload
{
    BackendAgentNativeTimerModifyKind kind = BackendAgentNativeTimerModifyKind::update;
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::string expectedBindingRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string backendNativeTimerId;
    std::string expectedCurrentFingerprint;
    std::string expectedSpecificationFingerprint;
    BackendAgentNativeTimerCreateSpecification specification;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentLocalProviderSelection localProviderSelection;
};
bool backendAgentNativeTimerModifyPayloadValid(
    const BackendAgentNativeTimerModifyPayload& payload);
std::string backendAgentNativeTimerModifyPayload(
    const BackendAgentNativeTimerModifyPayload& payload);
bool backendAgentNativeTimerModifyParsePayload(
    const std::string& encoded,
    BackendAgentNativeTimerModifyPayload& payload,
    std::string& reasonCode);
}
