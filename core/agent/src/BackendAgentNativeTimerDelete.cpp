#include "BackendAgentNativeTimerDelete.h"

#include <cstddef>

namespace vdrsuite::agent
{
namespace
{
constexpr std::size_t kMaxIdentityLength = 192;
constexpr std::size_t kMaxFingerprintLength = 512;
constexpr std::size_t kMaxEvidenceReferenceLength = 512;

bool identity(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxIdentityLength;
}

bool fingerprint(const std::string& value)
{
    return !value.empty() && value.size() <= kMaxFingerprintLength;
}

bool exactProviderSelection(
    const BackendAgentLocalProviderSelection& selection,
    const std::string& backendId,
    std::string& reasonCode)
{
    if (!backendAgentLocalProviderValidSelection(selection))
    {
        reasonCode = "invalid-provider-selection";
        return false;
    }
    if (selection.backendId != backendId ||
        selection.authorityDomain != kBackendAgentNativeTimerDeleteAuthorityDomain ||
        selection.providerId != kBackendAgentNativeTimerDeleteProviderId ||
        selection.providerKind != kBackendAgentNativeTimerDeleteProviderKind ||
        selection.requiredCapability != kBackendAgentNativeTimerDeleteCapability)
    {
        reasonCode = "provider-selection-mismatch";
        return false;
    }
    return true;
}
}

bool backendAgentNativeTimerDeleteValidCommand(
    const BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode)
{
    reasonCode.clear();
    if (!identity(command.commandId) || !fingerprint(command.requestFingerprint) ||
        !identity(command.operationId) || !identity(command.operationRevision) ||
        !identity(command.nativeTimerBindingId) ||
        !identity(command.expectedBindingRevision) ||
        !fingerprint(command.expectedNativeTimerFingerprint) ||
        !identity(command.timerAssignmentId) ||
        !identity(command.backendNativeTimerId) || !identity(command.jobId) ||
        !identity(command.attemptId) || command.claimEpoch == 0 ||
        !identity(command.backendId) || !identity(command.agentId) ||
        !identity(command.agentInstanceId) || command.backendGeneration == 0 ||
        command.controlPlaneClaimedAt <= 0)
    {
        reasonCode = "invalid-command-identity";
        return false;
    }
    return exactProviderSelection(
        command.localProviderSelection, command.backendId, reasonCode);
}

bool backendAgentNativeTimerDeleteEvidenceMatches(
    const BackendAgentNativeTimerDeleteEvidence& evidence,
    const BackendAgentNativeTimerDeleteCommand& command,
    std::string& reasonCode)
{
    if (!backendAgentNativeTimerDeleteValidCommand(command, reasonCode))
        return false;

    if (evidence.commandId != command.commandId ||
        evidence.requestFingerprint != command.requestFingerprint ||
        evidence.operationId != command.operationId ||
        evidence.operationRevision != command.operationRevision ||
        evidence.jobId != command.jobId || evidence.attemptId != command.attemptId ||
        evidence.claimEpoch != command.claimEpoch ||
        evidence.backendId != command.backendId || evidence.agentId != command.agentId ||
        evidence.agentInstanceId != command.agentInstanceId ||
        evidence.backendGeneration != command.backendGeneration ||
        evidence.providerInstanceEpoch !=
            command.localProviderSelection.providerInstanceEpoch)
    {
        reasonCode = "evidence-identity-mismatch";
        return false;
    }

    if (evidence.localStartingPersistedAt < command.controlPlaneClaimedAt ||
        evidence.completedAt < evidence.localStartingPersistedAt ||
        evidence.evidenceReference.empty() ||
        evidence.evidenceReference.size() > kMaxEvidenceReferenceLength)
    {
        reasonCode = "invalid-evidence-timing";
        return false;
    }

    switch (evidence.outcome)
    {
        case BackendAgentNativeTimerDeleteOutcomeCategory::rejectedWithoutEffect:
            if (evidence.dispatchStartedAt != 0)
            {
                reasonCode = "rejected-outcome-has-dispatch";
                return false;
            }
            break;
        case BackendAgentNativeTimerDeleteOutcomeCategory::acceptedUnverified:
        case BackendAgentNativeTimerDeleteOutcomeCategory::outcomeUnknown:
            if (evidence.dispatchStartedAt < evidence.localStartingPersistedAt ||
                evidence.dispatchStartedAt > evidence.completedAt)
            {
                reasonCode = "invalid-dispatch-boundary";
                return false;
            }
            break;
        default:
            reasonCode = "unknown-outcome";
            return false;
    }

    reasonCode.clear();
    return true;
}

}
