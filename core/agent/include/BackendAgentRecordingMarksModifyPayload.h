#pragma once

#include "BackendAgentRecordingMarksModify.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::agent
{

struct BackendAgentRecordingMarksModifyPayload
{
    BackendAgentRecordingMarksModifyKind kind =
        BackendAgentRecordingMarksModifyKind::add;
    std::string operationRevision;
    std::string recordingKey;
    std::string expectedMarksRevision;
    int sourceFrame = -1;
    int targetFrame = -1;
    std::vector<int> replacementFrames;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::int64_t controlPlaneClaimedAt = 0;
    BackendAgentLocalProviderSelection localProviderSelection;
};

bool backendAgentRecordingMarksModifyValidPayload(
    const BackendAgentRecordingMarksModifyPayload& payload,
    std::string& reasonCode);

std::string backendAgentRecordingMarksModifyPayload(
    const BackendAgentRecordingMarksModifyPayload& payload);

bool backendAgentRecordingMarksModifyParsePayload(
    const std::string& encoded,
    BackendAgentRecordingMarksModifyPayload& payload,
    std::string& reasonCode);

}
