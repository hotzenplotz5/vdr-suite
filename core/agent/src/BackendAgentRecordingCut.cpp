#include "BackendAgentRecordingCut.h"

#include <algorithm>

namespace vdrsuite::agent
{
namespace
{

bool exactProviderSelection(const BackendAgentRecordingCutCommand& command)
{
    const auto& selection = command.localProviderSelection;
    return backendAgentLocalProviderValidSelection(selection) &&
        selection.backendId == command.backendId &&
        selection.authorityDomain == kBackendAgentRecordingCutAuthorityDomain &&
        selection.providerId == kBackendAgentRecordingCutProviderId &&
        selection.providerKind == kBackendAgentRecordingCutProviderKind &&
        selection.requiredCapability == kBackendAgentRecordingCutCapability;
}

bool safeEvidenceReference(const std::string& value)
{
    return !value.empty() && backendAgentCommandSafeText(value, 512);
}

}

bool backendAgentRecordingCutRevisionTokenValid(const std::string& value)
{
    return value.size() == 32 &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return (character >= '0' && character <= '9') ||
                (character >= 'a' && character <= 'f');
        });
}

bool backendAgentRecordingCutValidCommand(
    const BackendAgentRecordingCutCommand& command,
    std::string& reasonCode)
{
    if (!backendAgentCommandSafeIdentifier(command.commandId) ||
        !backendAgentCommandSafeText(command.requestFingerprint, 512) ||
        !backendAgentCommandSafeIdentifier(command.operationId) ||
        !backendAgentCommandSafeIdentifier(command.operationRevision) ||
        !backendAgentRecordingCutRevisionTokenValid(command.recordingKey) ||
        !backendAgentRecordingCutRevisionTokenValid(
            command.expectedMarksRevision) ||
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
        reasonCode = "invalid_recording_cut_command";
        return false;
    }

    reasonCode.clear();
    return true;
}

bool backendAgentRecordingCutEvidenceMatches(
    const BackendAgentRecordingCutEvidence& evidence,
    const BackendAgentRecordingCutCommand& command,
    std::string& reasonCode)
{
    std::string commandReason;
    const bool dispatched =
        evidence.outcome != BackendAgentRecordingCutOutcomeCategory::rejectedWithoutEffect;
    if (!backendAgentRecordingCutValidCommand(command, commandReason) ||
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
        reasonCode = "recording_cut_evidence_mismatch";
        return false;
    }

    reasonCode.clear();
    return true;
}

}
