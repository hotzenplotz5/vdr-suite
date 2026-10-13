#pragma once

#include "HevcHlsKeyframeProbe.h"

#include <string>

// This is the narrow handoff from a completed, bounded external probe to the
// pure GOP selector. It deliberately does not execute ffprobe or attest HEVC
// IDR/CRA semantics: a caller must provide those independently verified facts.
struct HevcHlsProbeAttestation {
    bool subprocessExitedSuccessfully = false;
    bool subprocessFinishedBeforeDeadline = false;
    bool outputComplete = false;
    bool recordingStable = false;
    bool independentlyVerifiedHevcRandomAccess = false;
    std::string sourceFingerprintBefore;
    std::string sourceFingerprintAfter;
};

inline HevcHlsKeyframeEvidence attestHevcHlsProbe(
    const HevcHlsKeyframeProbeResult& parsed,
    const HevcHlsProbeAttestation& attestation)
{
    HevcHlsKeyframeEvidence evidence;
    const bool identityGood = !attestation.sourceFingerprintBefore.empty() &&
        attestation.sourceFingerprintBefore == attestation.sourceFingerprintAfter;
    const bool complete = parsed.parsed && !parsed.truncated &&
        attestation.subprocessExitedSuccessfully &&
        attestation.subprocessFinishedBeforeDeadline &&
        attestation.outputComplete;
    // Never pass partial or failed probes into the eligibility predicate.
    if (!complete || !identityGood || !attestation.recordingStable ||
        !attestation.independentlyVerifiedHevcRandomAccess) {
        return evidence;
    }
    evidence.probeComplete = true;
    evidence.sourceIdentityVerified = true;
    evidence.inputIsStableRecording = true;
    evidence.packetKeyframesVerified = true;
    evidence.keyframePtsSeconds = parsed.keyframePtsSeconds;
    return evidence;
}
