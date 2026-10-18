#pragma once

#include "EpgNativeTimerConflictGuard.h"
#include "EpgNativeTimerExecutionAuthority.h"
#include "SuiteBridgeNativeTimerCreateTransport.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"

#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{

// Promote ONLY an already typed, backend-originated, read-only NTPREVIEW
// assessment into an exact-specification admission proof. The native
// SuiteBridge callback, not a Device or HTTP handler, must have captured all
// existing VDR timers, channels and devices under read-only observation.
// This function never constructs positive evidence from LSCC or counts alone.
inline bool attestBackendNativeTimerPreview(
    const vdrsuite::agent::SuiteBridgeNativeTimerPreviewProjection& verified,
    const BackendAgentRecord& agent,
    const vdrsuite::agent::BackendAgentLocalProviderSelection& provider,
    vdrsuite::timers::TimerAssignmentPlanningBackendCandidate& candidate,
    std::int64_t receivedAt,
    ProspectiveNativeTimerConflictProof& proof)
{
    proof = {};
    candidate.conflict =
        vdrsuite::timers::TimerAssignmentPlanningConflictState::unknown;

    const std::string exactFingerprint =
        vdrsuite::timers::nativeTimerSpecificationFingerprint(
            candidate.desiredNativeTimerSpecification);
    const std::string digest =
        vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintToken(
            exactFingerprint);
    if (!candidate.desiredNativeTimerSpecificationPresent ||
        exactFingerprint.empty() || digest.empty() ||
        !verified.isolatedCandidateClear ||
        verified.proposedStart <= 0 ||
        verified.proposedStop <= verified.proposedStart ||
        verified.nativeObservedAt <= 0 ||
        receivedAt <= 0 || receivedAt < verified.nativeObservedAt ||
        receivedAt - verified.nativeObservedAt > 15 ||
        verified.specificationDigest != digest ||
        verified.providerInstanceEpoch != provider.providerInstanceEpoch ||
        verified.consideredTimers > 4096 ||
        verified.consideredDevices == 0 || verified.consideredDevices > 64 ||
        verified.evidenceRevision.size() > 160 ||
        !vdrsuite::agent::backendAgentNativeTimerDeleteFingerprintTokenValid(
            verified.evidenceRevision) ||
        !currentNativeTimerCreateExecutionAuthority(
            agent, provider, candidate, receivedAt))
        return false;

    // Both the candidate decision and the proof inherit the native reply's
    // exact provider fence and a maximum fifteen-second admission window.
    // This does NOT claim a general future-allocation scheduler. Overlapping
    // timers and indeterminate/recursive timers are rejected by the source.
    candidate.conflict =
        vdrsuite::timers::TimerAssignmentPlanningConflictState::confirmedClear;
    proof.current = true;
    proof.backendId = candidate.backendId;
    proof.backendGeneration = candidate.currentBackendGeneration;
    proof.executionAuthorityFence = candidate.executionAuthorityFence;
    proof.nativeSpecificationFingerprint = exactFingerprint;
    proof.evidenceRevision = verified.evidenceRevision;
    proof.observedAt = receivedAt;
    proof.expiresAt = receivedAt + 15;
    proof.proposedTimerIncluded = true;
    proof.allExistingTimersConsidered = true;
    proof.devicesConsidered = true;
    proof.conflictsFound = false;

    if (!prospectiveNativeTimerConflictClear(proof, candidate, receivedAt))
    {
        candidate.conflict =
            vdrsuite::timers::TimerAssignmentPlanningConflictState::unknown;
        proof = {};
        return false;
    }
    return true;
}

} // namespace vdrsuite::public_epg_timer
