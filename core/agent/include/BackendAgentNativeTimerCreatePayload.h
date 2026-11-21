#pragma once

#include "BackendAgentNativeTimerCreate.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerCreatePayload
{
    std::string operationRevision;
    std::string timerAssignmentId;
    std::string expectedAssignmentRevision;
    std::string expectedIntentRevision;
    std::uint64_t assignmentEpoch = 0;
    std::string nativeTimerBindingId;
    std::int64_t controlPlaneClaimedAt = 0;
    std::string expectedSpecificationFingerprint;
    BackendAgentNativeTimerCreateSpecification specification;
    BackendAgentLocalProviderSelection localProviderSelection;
};

bool backendAgentNativeTimerCreatePayloadValid(
    const BackendAgentNativeTimerCreatePayload& payload);
std::string backendAgentNativeTimerCreatePayload(
    const BackendAgentNativeTimerCreatePayload& payload);
bool backendAgentNativeTimerCreateParsePayload(
    const std::string& encoded,
    BackendAgentNativeTimerCreatePayload& payload,
    std::string& reasonCode);

} // namespace vdrsuite::agent
