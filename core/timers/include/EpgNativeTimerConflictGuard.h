#pragma once

#include "NativeTimerSpecification.h"
#include "TimerAssignmentPlanner.h"

#include <cstdint>
#include <string>

namespace vdrsuite::public_epg_timer
{
// A prospective conflict proof is a backend-native simulation for the
// proposed timer, NOT the current epgsearch LSCC inventory. The producer must
// attest its provenance; the Domain verifies exact scope and freshness only.
struct ProspectiveNativeTimerConflictProof
{
    bool current = false;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string executionAuthorityFence;
    std::string nativeSpecificationFingerprint;
    std::string evidenceRevision;
    std::int64_t observedAt = 0;
    std::int64_t expiresAt = 0;
    bool proposedTimerIncluded = false;
    bool allExistingTimersConsidered = false;
    bool devicesConsidered = false;
    bool conflictsFound = true;
};

inline bool prospectiveNativeTimerConflictClear(
    const ProspectiveNativeTimerConflictProof& proof,
    const vdrsuite::timers::TimerAssignmentPlanningBackendCandidate& candidate,
    std::int64_t now)
{
    // A prospective result ages out quickly and may never be fabricated
    // from a zero-length /searchtimers/conflicts.json or SVDRP LSCC.
    constexpr std::int64_t kMaximumEvidenceAgeSeconds = 15;
    const std::string expected =
        vdrsuite::timers::nativeTimerSpecificationFingerprint(
            candidate.desiredNativeTimerSpecification);
    return proof.current && proof.proposedTimerIncluded &&
        proof.allExistingTimersConsidered && proof.devicesConsidered &&
        !proof.conflictsFound && !expected.empty() &&
        proof.nativeSpecificationFingerprint == expected &&
        proof.backendId == candidate.backendId &&
        !proof.backendId.empty() &&
        proof.backendGeneration != 0 &&
        proof.backendGeneration == candidate.currentBackendGeneration &&
        !proof.executionAuthorityFence.empty() &&
        proof.executionAuthorityFence == candidate.executionAuthorityFence &&
        !proof.evidenceRevision.empty() &&
        proof.evidenceRevision.size() <= 160 &&
        proof.observedAt > 0 && now >= proof.observedAt &&
        proof.expiresAt >= now &&
        proof.expiresAt >= proof.observedAt &&
        proof.expiresAt - proof.observedAt <= kMaximumEvidenceAgeSeconds &&
        now - proof.observedAt <= kMaximumEvidenceAgeSeconds &&
        candidate.conflict ==
            vdrsuite::timers::TimerAssignmentPlanningConflictState::confirmedClear;
}

} // namespace vdrsuite::public_epg_timer
