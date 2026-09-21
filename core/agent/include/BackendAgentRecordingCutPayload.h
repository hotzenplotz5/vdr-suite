#pragma once

#include "BackendAgentRecordingCut.h"

#include <cstdint>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentRecordingCutPayload
{
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentLocalProviderSelection localProviderSelection;
};

bool backendAgentRecordingCutValidPayload(
    const BackendAgentRecordingCutPayload& payload,
    std::string& reasonCode);

std::string backendAgentRecordingCutPayload(
    const BackendAgentRecordingCutPayload& payload);

bool backendAgentRecordingCutParsePayload(
    const std::string& encoded,
    BackendAgentRecordingCutPayload& payload,
    std::string& reasonCode);

}
