#include "BackendAgentRecordingMarksModify.h"

#include <algorithm>

namespace vdrsuite::agent
{
namespace
{

bool exactProviderSelection(const BackendAgentRecordingMarksModifyCommand& command)
{
    const auto& selection = command.localProviderSelection;
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.backendId == command.backendId &&
        selection.authorityDomain ==
            kBackendAgentRecordingMarksModifyAuthorityDomain &&
        selection.providerId == kBackendAgentRecordingMarksModifyProviderId &&
        selection.providerKind == kBackendAgentRecordingMarksModifyProviderKind &&
        selection.requiredCapability ==
            kBackendAgentRecordingMarksModifyCapability;
}

bool nonNegativeFrames(const std::vector<int>& frames)
{
    return std::all_of(frames.begin(), frames.end(), [](int frame) {
        return frame >= 0;
    });
}

bool safeEvidenceReference(const std::string& value)
{
    return !value.empty() && backendAgentCommandSafeText(value, 512);
}

}

const char* backendAgentRecordingMarksModifyKindName(
    BackendAgentRecordingMarksModifyKind kind)
{
    switch (kind)
    {
        case BackendAgentRecordingMarksModifyKind::add: return "add";
        case BackendAgentRecordingMarksModifyKind::deleteMark: return "delete";
        case BackendAgentRecordingMarksModifyKind::move: return "move";
        case BackendAgentRecordingMarksModifyKind::reset: return "reset";
        case BackendAgentRecordingMarksModifyKind::replace: return "replace";
    }
    return "invalid";
}

bool backendAgentRecordingMarksModifyFrameShapeValid(
    BackendAgentRecordingMarksModifyKind kind,
    int sourceFrame,
    int targetFrame,
    const std::vector<int>& replacementFrames)
{
    if (replacementFrames.size() >
            kBackendAgentRecordingMarksMaximumReplacementFrames ||
        !nonNegativeFrames(replacementFrames))
    {
        return false;
    }

    switch (kind)
    {
        case BackendAgentRecordingMarksModifyKind::add:
            return sourceFrame < 0 && targetFrame >= 0 &&
                replacementFrames.empty();
        case BackendAgentRecordingMarksModifyKind::deleteMark:
            return sourceFrame >= 0 && targetFrame < 0 &&
                replacementFrames.empty();
        case BackendAgentRecordingMarksModifyKind::move:
            return sourceFrame >= 0 && targetFrame >= 0 &&
                replacementFrames.empty();
        case BackendAgentRecordingMarksModifyKind::reset:
            return sourceFrame < 0 && targetFrame < 0 &&
                replacementFrames.empty();
        case BackendAgentRecordingMarksModifyKind::replace:
            return sourceFrame < 0 && targetFrame < 0 &&
                !replacementFrames.empty();
    }
    return false;
}

bool backendAgentRecordingMarksModifyRevisionTokenValid(
    const std::string& value)
{
    return value.size() == 32 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

bool backendAgentRecordingMarksModifyValidCommand(
    const BackendAgentRecordingMarksModifyCommand& command,
    std::string& reasonCode)
{
    if (std::string(backendAgentRecordingMarksModifyKindName(command.kind)) ==
            "invalid" ||
        !backendAgentCommandSafeIdentifier(command.commandId) ||
        !backendAgentCommandSafeText(command.requestFingerprint, 512) ||
        !backendAgentCommandSafeIdentifier(command.operationId) ||
        !backendAgentCommandSafeIdentifier(command.operationRevision) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(
            command.recordingKey) ||
        !backendAgentRecordingMarksModifyRevisionTokenValid(
            command.expectedMarksRevision) ||
        !backendAgentRecordingMarksModifyFrameShapeValid(
            command.kind,
            command.sourceFrame,
            command.targetFrame,
            command.replacementFrames) ||
        !backendAgentCommandSafeIdentifier(command.jobId) ||
        !backendAgentCommandSafeIdentifier(command.attemptId) ||
        command.claimEpoch == 0 ||
        !backendAgentCommandSafeIdentifier(command.backendId) ||
        !backendAgentCommandSafeIdentifier(command.agentId) ||
        !backendAgentCommandSafeIdentifier(command.agentInstanceId) ||
        command.backendGeneration == 0 ||
        command.controlPlaneClaimedAt <= 0 ||
        !exactProviderSelection(command))
    {
        reasonCode = "invalid_recording_marks_modify_command";
        return false;
    }

    reasonCode.clear();
    return true;
}

bool backendAgentRecordingMarksModifyEvidenceMatches(
    const BackendAgentRecordingMarksModifyEvidence& evidence,
    const BackendAgentRecordingMarksModifyCommand& command,
    std::string& reasonCode)
{
    std::string commandReason;
    const bool dispatched =
        evidence.outcome !=
            BackendAgentRecordingMarksModifyOutcomeCategory::rejectedWithoutEffect;
    if (!backendAgentRecordingMarksModifyValidCommand(command, commandReason) ||
        evidence.commandId != command.commandId ||
        evidence.requestFingerprint != command.requestFingerprint ||
        evidence.operationId != command.operationId ||
        evidence.operationRevision != command.operationRevision ||
        evidence.jobId != command.jobId ||
        evidence.attemptId != command.attemptId ||
        evidence.claimEpoch != command.claimEpoch ||
        evidence.backendId != command.backendId ||
        evidence.agentId != command.agentId ||
        evidence.agentInstanceId != command.agentInstanceId ||
        evidence.backendGeneration != command.backendGeneration ||
        evidence.providerInstanceEpoch !=
            command.localProviderSelection.providerInstanceEpoch ||
        evidence.localStartingPersistedAt < command.controlPlaneClaimedAt ||
        evidence.localStartingPersistedAt <= 0 ||
        (dispatched &&
            evidence.dispatchStartedAt < evidence.localStartingPersistedAt) ||
        (!dispatched && evidence.dispatchStartedAt != 0) ||
        evidence.completedAt < evidence.localStartingPersistedAt ||
        (dispatched && evidence.completedAt < evidence.dispatchStartedAt) ||
        !safeEvidenceReference(evidence.evidenceReference))
    {
        reasonCode = "recording_marks_modify_evidence_mismatch";
        return false;
    }

    reasonCode.clear();
    return true;
}

}
