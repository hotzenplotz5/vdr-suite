#pragma once

#include "MediaCapabilities.h"

#include <algorithm>
#include <cmath>
#include <vector>

// Pure decision layer only. Do not infer GOP from codec, frame rate, HDR,
// dimensions or r_frame_rate. The caller must provide verified source packet
// keyframe timestamps, an immutable source fingerprint and a bounded probe.
// No production code currently supplies this evidence.
struct HevcHlsKeyframeEvidence
{
    bool probeComplete = false;
    bool sourceIdentityVerified = false;
    bool inputIsStableRecording = false;
    bool packetKeyframesVerified = false;
    std::vector<double> keyframePtsSeconds;
};

enum class HevcHlsSegmentDecision
{
    Legacy,
    ExperimentalKeyframe
};

inline HevcHlsSegmentDecision decideHevcHlsSegmentPolicy(
    const MediaPresentationProfile& profile,
    const HevcHlsKeyframeEvidence& evidence)
{
    if (!profile.available ||
        profile.protocol != MediaDeliveryProtocol::Hls ||
        profile.container != MediaContainer::Fmp4 ||
        profile.videoAction != MediaTrackAction::Copy ||
        profile.targetVideoCodec != MediaCodec::H265 ||
        !evidence.probeComplete || !evidence.sourceIdentityVerified ||
        !evidence.inputIsStableRecording ||
        !evidence.packetKeyframesVerified ||
        evidence.keyframePtsSeconds.size() < 6 ||
        evidence.keyframePtsSeconds.size() > 1024) {
        return HevcHlsSegmentDecision::Legacy;
    }

    // Require at least five consecutive, well-ordered, short GOP intervals.
    // Source-keyframe flag is not by itself proof of a HEVC IDR/CRA boundary:
    // upstream packet-level validation is explicitly required.
    const auto& pts = evidence.keyframePtsSeconds;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        if (!std::isfinite(pts[i]) || pts[i] < 0.0) {
            return HevcHlsSegmentDecision::Legacy;
        }
        if (i != 0) {
            const double gap = pts[i] - pts[i - 1];
            if (!(gap >= 0.25 && gap <= 2.0)) {
                return HevcHlsSegmentDecision::Legacy;
            }
        }
    }
    return HevcHlsSegmentDecision::ExperimentalKeyframe;
}
