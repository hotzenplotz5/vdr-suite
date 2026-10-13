#include "HevcHlsProbeAttestation.h"

#include <cassert>

int main()
{
    MediaPresentationProfile p;
    p.available = true;
    p.protocol = MediaDeliveryProtocol::Hls;
    p.container = MediaContainer::Fmp4;
    p.videoAction = MediaTrackAction::Copy;
    p.targetVideoCodec = MediaCodec::H265;

    const auto parsed = parseHevcHlsKeyframeProbe(
        "1.400,K__\n2.399,K__\n3.398,K__\n4.397,K__\n5.396,K__\n6.395,K__\n");
    assert(parsed.parsed);
    HevcHlsProbeAttestation a;
    a.subprocessExitedSuccessfully = true;
    a.subprocessFinishedBeforeDeadline = true;
    a.outputComplete = true;
    a.recordingStable = true;
    a.independentlyVerifiedHevcRandomAccess = true;
    a.sourceFingerprintBefore = "fp-1";
    a.sourceFingerprintAfter = "fp-1";

    auto result = attestHevcHlsProbe(parsed, a);
    assert(decideHevcHlsSegmentPolicy(p, result) ==
           HevcHlsSegmentDecision::ExperimentalKeyframe);

    a.subprocessExitedSuccessfully = false;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);
    a.subprocessExitedSuccessfully = true;
    a.subprocessFinishedBeforeDeadline = false;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);
    a.subprocessFinishedBeforeDeadline = true;
    a.outputComplete = false;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);
    a.outputComplete = true;
    a.sourceFingerprintAfter = "changed";
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);
    a.sourceFingerprintAfter = "fp-1";
    a.recordingStable = false;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);
    a.recordingStable = true;
    a.independentlyVerifiedHevcRandomAccess = false;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(parsed, a)) ==
           HevcHlsSegmentDecision::Legacy);

    a.independentlyVerifiedHevcRandomAccess = true;
    auto incomplete = parsed;
    incomplete.truncated = true;
    assert(decideHevcHlsSegmentPolicy(p, attestHevcHlsProbe(incomplete, a)) ==
           HevcHlsSegmentDecision::Legacy);
    return 0;
}
