#include "HevcHlsSegmentPolicy.h"

#include <cassert>
#include <limits>

namespace {
MediaPresentationProfile profile()
{
    MediaPresentationProfile p;
    p.available = true;
    p.protocol = MediaDeliveryProtocol::Hls;
    p.container = MediaContainer::Fmp4;
    p.videoAction = MediaTrackAction::Copy;
    p.targetVideoCodec = MediaCodec::H265;
    return p;
}

HevcHlsKeyframeEvidence medicus()
{
    HevcHlsKeyframeEvidence e;
    e.probeComplete = true;
    e.sourceIdentityVerified = true;
    e.inputIsStableRecording = true;
    e.packetKeyframesVerified = true;
    e.keyframePtsSeconds = {1.4, 2.399, 3.398, 4.397, 5.396, 6.395, 7.394};
    return e;
}
} // namespace

int main()
{
    const auto p = profile();
    auto e = medicus();
    assert(decideHevcHlsSegmentPolicy(p, e) ==
           HevcHlsSegmentDecision::ExperimentalKeyframe);

    e.keyframePtsSeconds = {0.063, 10.469, 20.896, 31.323, 41.75, 52.177};
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);

    e = medicus();
    e.probeComplete = false;
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.sourceIdentityVerified = false;
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.inputIsStableRecording = false;
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.packetKeyframesVerified = false;
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);

    e = medicus();
    e.keyframePtsSeconds = {1.0, 2.0, 3.0};
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.keyframePtsSeconds[2] = e.keyframePtsSeconds[1];
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.keyframePtsSeconds[2] = std::numeric_limits<double>::quiet_NaN();
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);
    e = medicus();
    e.keyframePtsSeconds[2] += 5.0;
    assert(decideHevcHlsSegmentPolicy(p, e) == HevcHlsSegmentDecision::Legacy);

    e = medicus();
    auto changed = p;
    changed.targetVideoCodec = MediaCodec::H264;
    assert(decideHevcHlsSegmentPolicy(changed, e) == HevcHlsSegmentDecision::Legacy);
    changed = p;
    changed.container = MediaContainer::MpegTs;
    assert(decideHevcHlsSegmentPolicy(changed, e) == HevcHlsSegmentDecision::Legacy);
    changed = p;
    changed.videoAction = MediaTrackAction::Transcode;
    assert(decideHevcHlsSegmentPolicy(changed, e) == HevcHlsSegmentDecision::Legacy);
    changed = p;
    changed.available = false;
    assert(decideHevcHlsSegmentPolicy(changed, e) == HevcHlsSegmentDecision::Legacy);
    return 0;
}
