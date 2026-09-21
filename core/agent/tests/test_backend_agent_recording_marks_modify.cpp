#include "../include/BackendAgentRecordingMarksModifyPayload.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace vdrsuite::agent;

namespace
{

void require(bool value, const char* message)
{
    if (!value)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

BackendAgentLocalProviderSelection selection()
{
    BackendAgentLocalProviderSelection value;
    value.backendId = "backend_1";
    value.authorityDomain = kBackendAgentRecordingMarksModifyAuthorityDomain;
    value.providerId = kBackendAgentRecordingMarksModifyProviderId;
    value.providerKind = kBackendAgentRecordingMarksModifyProviderKind;
    value.ownershipGeneration = 3;
    value.providerInstanceEpoch = "pie_1";
    value.providerGeneration = 4;
    value.capabilityRevision = 2;
    value.requiredCapability = kBackendAgentRecordingMarksModifyCapability;
    return value;
}

BackendAgentRecordingMarksModifyPayload payload(
    BackendAgentRecordingMarksModifyKind kind)
{
    BackendAgentRecordingMarksModifyPayload value;
    value.kind = kind;
    value.operationRevision = "oprev_1";
    value.recordingKey = "0123456789abcdef0123456789abcdef";
    value.expectedMarksRevision = "fedcba9876543210fedcba9876543210";
    value.backendId = "backend_1";
    value.backendGeneration = 7;
    value.controlPlaneClaimedAt = 100;
    value.localProviderSelection = selection();
    switch (kind)
    {
        case BackendAgentRecordingMarksModifyKind::add:
            value.targetFrame = 125;
            break;
        case BackendAgentRecordingMarksModifyKind::deleteMark:
            value.sourceFrame = 125;
            break;
        case BackendAgentRecordingMarksModifyKind::move:
            value.sourceFrame = 125;
            value.targetFrame = 250;
            break;
        case BackendAgentRecordingMarksModifyKind::reset:
            break;
        case BackendAgentRecordingMarksModifyKind::replace:
            value.replacementFrames = {125, 250, 375};
            break;
    }
    return value;
}

BackendAgentRecordingMarksModifyCommand command(
    const BackendAgentRecordingMarksModifyPayload& payloadValue)
{
    BackendAgentRecordingMarksModifyCommand value;
    value.kind = payloadValue.kind;
    value.commandId = "cmd_1";
    value.requestFingerprint =
        "sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    value.operationId = "op_1";
    value.operationRevision = payloadValue.operationRevision;
    value.recordingKey = payloadValue.recordingKey;
    value.expectedMarksRevision = payloadValue.expectedMarksRevision;
    value.sourceFrame = payloadValue.sourceFrame;
    value.targetFrame = payloadValue.targetFrame;
    value.replacementFrames = payloadValue.replacementFrames;
    value.jobId = "job_1";
    value.attemptId = "attempt_1";
    value.claimEpoch = 1;
    value.backendId = payloadValue.backendId;
    value.agentId = "agent_1";
    value.agentInstanceId = "instance_1";
    value.backendGeneration = payloadValue.backendGeneration;
    value.controlPlaneClaimedAt = payloadValue.controlPlaneClaimedAt;
    value.localProviderSelection = payloadValue.localProviderSelection;
    return value;
}

}

int main()
{
    for (const auto kind : {
             BackendAgentRecordingMarksModifyKind::add,
             BackendAgentRecordingMarksModifyKind::deleteMark,
             BackendAgentRecordingMarksModifyKind::move,
             BackendAgentRecordingMarksModifyKind::reset,
             BackendAgentRecordingMarksModifyKind::replace})
    {
        const auto value = payload(kind);
        std::string reason;
        require(
            backendAgentRecordingMarksModifyValidPayload(value, reason),
            "valid marks payload rejected");
        const std::string encoded = backendAgentRecordingMarksModifyPayload(value);
        require(!encoded.empty(), "marks payload serialization failed");
        BackendAgentRecordingMarksModifyPayload parsed;
        require(
            backendAgentRecordingMarksModifyParsePayload(encoded, parsed, reason),
            "marks payload parse failed");
        require(
            backendAgentRecordingMarksModifyPayload(parsed) == encoded,
            "marks payload did not roundtrip canonically");
        require(
            parsed.kind == kind &&
                parsed.recordingKey == value.recordingKey &&
                parsed.expectedMarksRevision == value.expectedMarksRevision &&
                parsed.sourceFrame == value.sourceFrame &&
                parsed.targetFrame == value.targetFrame &&
                parsed.replacementFrames == value.replacementFrames &&
                backendAgentLocalProviderSameFence(
                    parsed.localProviderSelection,
                    value.localProviderSelection),
            "marks payload roundtrip changed request semantics");
        require(
            backendAgentRecordingMarksModifyValidCommand(command(value), reason),
            "valid marks command rejected");
    }

    auto invalidRevision = payload(BackendAgentRecordingMarksModifyKind::add);
    invalidRevision.expectedMarksRevision = "not-a-revision";
    std::string reason;
    require(
        !backendAgentRecordingMarksModifyValidPayload(invalidRevision, reason),
        "invalid expected marks revision accepted");

    auto invalidReset = payload(BackendAgentRecordingMarksModifyKind::reset);
    invalidReset.targetFrame = 100;
    require(
        !backendAgentRecordingMarksModifyValidPayload(invalidReset, reason),
        "reset with browser frame accepted");

    auto invalidReplace = payload(BackendAgentRecordingMarksModifyKind::replace);
    invalidReplace.replacementFrames.clear();
    require(
        !backendAgentRecordingMarksModifyValidPayload(invalidReplace, reason),
        "empty replacement accepted");

    auto staleProvider = payload(BackendAgentRecordingMarksModifyKind::move);
    staleProvider.localProviderSelection.authorityDomain = "vdr.timer";
    require(
        !backendAgentRecordingMarksModifyValidPayload(staleProvider, reason),
        "wrong provider authority accepted");

    const auto canonical = backendAgentRecordingMarksModifyPayload(
        payload(BackendAgentRecordingMarksModifyKind::replace));
    BackendAgentRecordingMarksModifyPayload parsed;
    require(
        !backendAgentRecordingMarksModifyParsePayload(
            canonical + "garbage", parsed, reason),
        "non-canonical trailing data accepted");

    std::vector<int> tooMany(
        kBackendAgentRecordingMarksMaximumReplacementFrames + 1, 10);
    require(
        !backendAgentRecordingMarksModifyFrameShapeValid(
            BackendAgentRecordingMarksModifyKind::replace,
            -1,
            -1,
            tooMany),
        "oversized replacement accepted");

    std::cout << "recording marks Agent command contract tests passed\n";
}
