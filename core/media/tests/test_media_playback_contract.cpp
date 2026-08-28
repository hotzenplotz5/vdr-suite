#include "MediaPlaybackContract.h"

#include <cassert>
#include <string>

int main()
{
    MediaPlaybackTrackCapabilities tracks;
    tracks.audioSelectionSupported = true;
    tracks.subtitleSelectionSupported = true;
    tracks.subtitleOffSupported = true;

    const MediaPlaybackContract progressive = MediaPlaybackContractFactory::recording(
        "progressive-fmp4",
        false,
        120,
        5530,
        120,
        true,
        false,
        tracks);
    assert(progressive.resourceMode == MediaPlaybackResourceMode::Recording);
    assert(progressive.seek.supported.has_value() && *progressive.seek.supported);
    assert(progressive.seek.mode == MediaPlaybackSeekMode::InSessionReposition);
    assert(progressive.seek.windowStartSeconds.has_value() && *progressive.seek.windowStartSeconds == 0);
    assert(progressive.seek.windowEndSeconds.has_value() && *progressive.seek.windowEndSeconds == 5530);
    assert(progressive.restartSupported.has_value() && *progressive.restartSupported);

    const std::string progressiveJson = MediaPlaybackContractFactory::json(progressive);
    assert(progressiveJson.find("\"contractVersion\":1") != std::string::npos);
    assert(progressiveJson.find("\"resourceMode\":\"recording\"") != std::string::npos);
    assert(progressiveJson.find("\"presentationProfileId\":\"progressive-fmp4\"") != std::string::npos);
    assert(progressiveJson.find("\"presentationBasePositionSeconds\":120") != std::string::npos);
    assert(progressiveJson.find("\"mode\":\"in-session-reposition\"") != std::string::npos);
    assert(progressiveJson.find("\"audioSelection\":{\"supported\":true}") != std::string::npos);
    assert(progressiveJson.find("\"subtitleSelection\":{\"supported\":true}") != std::string::npos);
    assert(progressiveJson.find("\"subtitleOff\":{\"supported\":true}") != std::string::npos);
    assert(progressiveJson.find("\"generation\":null") != std::string::npos);
    assert(progressiveJson.find("\"state\":null") != std::string::npos);
    assert(progressiveJson.find("\"failure\":null") != std::string::npos);
    assert(progressiveJson.find("videoEncoder") == std::string::npos);
    assert(progressiveJson.find("hardwareDevice") == std::string::npos);
    assert(progressiveJson.find("provider") == std::string::npos);
    assert(progressiveJson.find("ffmpeg") == std::string::npos);

    MediaPlaybackContract continuity = progressive;
    continuity.continuityGeneration = 7;
    continuity.continuityState = "stable";
    const std::string continuityJson = MediaPlaybackContractFactory::json(continuity);
    assert(continuityJson.find("\"continuity\":{\"generation\":7,\"state\":\"stable\"}") != std::string::npos);
    assert(continuityJson.find("routeEpoch") == std::string::npos);
    assert(continuityJson.find("lifecycleRevision") == std::string::npos);

    const std::string progressiveLegacy =
        MediaPlaybackContractFactory::legacyPlaybackJson(progressive);
    assert(progressiveLegacy.find("\"seek\":{\"supported\":true") != std::string::npos);
    assert(progressiveLegacy.find("\"resume\":{\"supported\":true") != std::string::npos);

    const MediaPlaybackContract hls = MediaPlaybackContractFactory::recording(
        "hls-fmp4",
        false,
        2832,
        5530,
        2832,
        true,
        false,
        tracks);
    assert(hls.seek.supported.has_value() && *hls.seek.supported);
    assert(hls.seek.mode == MediaPlaybackSeekMode::ReplacementSessionRestart);
    assert(hls.presentationBasePositionSeconds.has_value() && *hls.presentationBasePositionSeconds == 2832);
    const std::string hlsJson = MediaPlaybackContractFactory::json(hls);
    assert(hlsJson.find("\"mode\":\"replacement-session-restart\"") != std::string::npos);
    assert(hlsJson.find("\"positionSeconds\":2832") != std::string::npos);
    assert(hlsJson.find("\"presentationBasePositionSeconds\":2832") != std::string::npos);
    const std::string hlsLegacy =
        MediaPlaybackContractFactory::legacyPlaybackJson(hls);
    assert(hlsLegacy.find("\"seek\":{\"supported\":false") != std::string::npos);
    assert(hlsLegacy.find("\"resume\":{\"supported\":true") != std::string::npos);

    const MediaPlaybackContract preparingHls = MediaPlaybackContractFactory::recording(
        "hls-fmp4",
        false,
        0,
        0,
        0,
        false,
        true);
    assert(preparingHls.seek.supported.has_value() && !*preparingHls.seek.supported);
    assert(preparingHls.seek.preparing.has_value() && *preparingHls.seek.preparing);
    assert(preparingHls.seek.mode == MediaPlaybackSeekMode::ReplacementSessionRestart);
    assert(preparingHls.restartSupported.has_value() && !*preparingHls.restartSupported);
    assert(preparingHls.restartPreparing.has_value() && *preparingHls.restartPreparing);

    const MediaPlaybackContract growing = MediaPlaybackContractFactory::recording(
        "hls-fmp4",
        true,
        0,
        5530,
        0,
        true,
        true,
        tracks);
    assert(growing.resourceMode == MediaPlaybackResourceMode::GrowingRecording);
    assert(!growing.durationSeconds.has_value());
    assert(growing.seek.supported.has_value() && !*growing.seek.supported);
    assert(growing.seek.preparing.has_value() && !*growing.seek.preparing);
    assert(growing.restartSupported.has_value() && !*growing.restartSupported);
    assert(growing.restartPreparing.has_value() && !*growing.restartPreparing);
    const std::string growingJson = MediaPlaybackContractFactory::json(growing);
    assert(growingJson.find("\"resourceMode\":\"growing-recording\"") != std::string::npos);
    assert(growingJson.find("\"durationSeconds\":null") != std::string::npos);

    MediaPlaybackTrackCapabilities partialTracks;
    partialTracks.audioSelectionSupported = true;
    const MediaPlaybackContract partial = MediaPlaybackContractFactory::recordingFromLegacy(
        "hls-fmp4",
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        partialTracks);
    const std::string partialJson = MediaPlaybackContractFactory::json(partial);
    assert(partialJson.find("\"positionSeconds\":null") != std::string::npos);
    assert(partialJson.find("\"durationSeconds\":null") != std::string::npos);
    assert(partialJson.find("\"supported\":null,\"mode\":\"replacement-session-restart\"") != std::string::npos);
    assert(partialJson.find("\"audioSelection\":{\"supported\":true}") != std::string::npos);
    assert(partialJson.find("\"subtitleSelection\":{\"supported\":null}") != std::string::npos);

    const MediaPlaybackContract live = MediaPlaybackContractFactory::live("progressive-fmp4");
    assert(live.resourceMode == MediaPlaybackResourceMode::Live);
    assert(live.seek.mode == MediaPlaybackSeekMode::Unsupported);
    assert(live.seek.supported.has_value() && !*live.seek.supported);
    const std::string liveJson = MediaPlaybackContractFactory::json(live);
    assert(liveJson.find("\"resourceMode\":\"live\"") != std::string::npos);
    assert(liveJson.find("\"mode\":\"unsupported\"") != std::string::npos);
    assert(liveJson.find("\"positionSeconds\":null") != std::string::npos);

    const auto authorization =
        MediaPlaybackContractFactory::classifyFailure("media_access_denied");
    assert(authorization.has_value());
    assert(authorization->category == "authorization");
    assert(authorization->origin == "gateway");
    assert(authorization->stage == "access-authorization");
    assert(authorization->terminal);
    assert(authorization->recoveryClass == "new-authorization");
    assert(authorization->reasonCode == "media_access_denied");

    const auto source =
        MediaPlaybackContractFactory::classifyFailure("media_source_unsupported");
    assert(source.has_value());
    assert(source->category == "source");
    assert(source->origin == "control-plane");
    assert(source->stage == "source-probe");
    assert(source->terminal);
    assert(source->reasonCode == "media_source_unsupported");

    const auto exactResume =
        MediaPlaybackContractFactory::classifyFailure("recording_resume_profile_not_supported");
    assert(exactResume.has_value());
    assert(exactResume->category == "adaptation");
    assert(exactResume->stage == "presentation-selection");
    assert(exactResume->terminal);
    assert(exactResume->reasonCode == "recording_resume_profile_not_supported");

    const auto worker =
        MediaPlaybackContractFactory::classifyFailure("media_worker_start_failed");
    assert(worker.has_value());
    assert(worker->category == "transport");
    assert(worker->origin == "media-worker");
    assert(worker->stage == "provision-start");
    assert(worker->terminal);
    assert(worker->reasonCode == "media_worker_start_failed");

    const auto timeline =
        MediaPlaybackContractFactory::classifyFailure("recording_index_update_failed");
    assert(timeline.has_value());
    assert(timeline->category == "timeline");
    assert(timeline->origin == "control-plane");
    assert(timeline->stage == "timeline-preparation");
    assert(!timeline->terminal);
    assert(timeline->recoveryClass == "none");
    assert(timeline->reasonCode == "recording_index_update_failed");

    assert(!MediaPlaybackContractFactory::classifyFailure(
        "unobserved_future_failure").has_value());

    const MediaPlaybackContract failed = MediaPlaybackContractFactory::failed(
        MediaPlaybackResourceMode::Recording,
        "media_worker_start_failed");
    const std::string failedJson = MediaPlaybackContractFactory::json(failed);
    assert(failedJson.find(
        "\"failure\":{\"category\":\"transport\",\"origin\":\"media-worker\","
        "\"stage\":\"provision-start\",\"terminal\":true,"
        "\"recoveryClass\":\"new-authorized-contract\","
        "\"reasonCode\":\"media_worker_start_failed\"}") != std::string::npos);
    assert(failedJson.find("routeEpoch") == std::string::npos);
    assert(failedJson.find("lifecycleRevision") == std::string::npos);

    return 0;
}
