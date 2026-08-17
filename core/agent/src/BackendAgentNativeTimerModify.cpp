#include "BackendAgentNativeTimerModify.h"

#include "BackendAgentNativeTimerDeleteFingerprint.h"

#include <cstddef>

namespace vdrsuite::agent
{
namespace {
constexpr std::size_t MaxIdentity=192;
bool identity(const std::string& value) {
    return !value.empty() && value.size()<=MaxIdentity;
}
bool exactSelection(const BackendAgentNativeTimerModifyCommand& command) {
    const auto& s=command.localProviderSelection;
    return backendAgentLocalProviderValidSelection(s) &&
        s.backendId==command.backendId &&
        s.authorityDomain==kBackendAgentNativeTimerModifyAuthorityDomain &&
        s.providerId==kBackendAgentNativeTimerModifyProviderId &&
        s.providerKind==kBackendAgentNativeTimerModifyProviderKind &&
        s.requiredCapability==backendAgentNativeTimerModifyCapability(command.kind);
}
}

const char* backendAgentNativeTimerModifyKindName(BackendAgentNativeTimerModifyKind kind)
{
    switch (kind) {
        case BackendAgentNativeTimerModifyKind::update: return "update";
        case BackendAgentNativeTimerModifyKind::toggle: return "toggle";
    }
    return "invalid";
}
const char* backendAgentNativeTimerModifyCapability(BackendAgentNativeTimerModifyKind kind)
{
    switch (kind) {
        case BackendAgentNativeTimerModifyKind::update:
            return kBackendAgentNativeTimerUpdateCapability;
        case BackendAgentNativeTimerModifyKind::toggle:
            return kBackendAgentNativeTimerToggleCapability;
    }
    return "invalid";
}

bool backendAgentNativeTimerModifyValidCommand(
    const BackendAgentNativeTimerModifyCommand& c, std::string& reason)
{
    if (std::string(backendAgentNativeTimerModifyKindName(c.kind))=="invalid" ||
        !identity(c.commandId) || c.requestFingerprint.empty() ||
        c.requestFingerprint.size()>512 || !identity(c.operationId) ||
        !identity(c.operationRevision) || !identity(c.timerAssignmentId) ||
        !identity(c.expectedAssignmentRevision) || !identity(c.expectedIntentRevision) ||
        c.assignmentEpoch==0 || !identity(c.nativeTimerBindingId) ||
        !identity(c.expectedBindingRevision) || !identity(c.backendNativeTimerId) ||
        !backendAgentNativeTimerDeleteFingerprintTokenValid(c.expectedCurrentFingerprint) ||
        !backendAgentNativeTimerCreateSpecificationValid(c.specification) ||
        c.expectedSpecificationFingerprint !=
            backendAgentNativeTimerCreateSpecificationFingerprint(c.specification) ||
        !identity(c.jobId) || !identity(c.attemptId) || c.claimEpoch==0 ||
        !identity(c.backendId) || !identity(c.agentId) ||
        !identity(c.agentInstanceId) || c.backendGeneration==0 ||
        c.controlPlaneClaimedAt<=0 || !exactSelection(c))
    {
        reason="invalid_native_timer_modify_command";
        return false;
    }
    reason.clear();
    return true;
}

bool backendAgentNativeTimerModifyEvidenceMatches(
    const BackendAgentNativeTimerModifyEvidence& e,
    const BackendAgentNativeTimerModifyCommand& c, std::string& reason)
{
    if (!backendAgentNativeTimerModifyValidCommand(c,reason)) return false;
    if (e.commandId!=c.commandId || e.requestFingerprint!=c.requestFingerprint ||
        e.operationId!=c.operationId || e.operationRevision!=c.operationRevision ||
        e.jobId!=c.jobId || e.attemptId!=c.attemptId ||
        e.claimEpoch!=c.claimEpoch || e.backendId!=c.backendId ||
        e.agentId!=c.agentId || e.agentInstanceId!=c.agentInstanceId ||
        e.backendGeneration!=c.backendGeneration ||
        e.providerInstanceEpoch!=c.localProviderSelection.providerInstanceEpoch ||
        e.localStartingPersistedAt<c.controlPlaneClaimedAt ||
        e.completedAt<e.localStartingPersistedAt ||
        e.evidenceReference.empty() || e.evidenceReference.size()>512)
    {
        reason="native_timer_modify_evidence_mismatch";
        return false;
    }
    if (e.outcome==BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect) {
        if (e.dispatchStartedAt!=0) {
            reason="native_timer_modify_rejection_has_dispatch";
            return false;
        }
    } else if (e.dispatchStartedAt<e.localStartingPersistedAt ||
               e.dispatchStartedAt>e.completedAt) {
        reason="native_timer_modify_evidence_timing_invalid";
        return false;
    }
    reason.clear();
    return true;
}
}
