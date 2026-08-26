#pragma once

#include <string>

enum class MediaPlaybackResourceMode
{
    CompletedRecording,
    GrowingRecording,
    Live
};

enum class MediaPlaybackSeekMode
{
    Unsupported,
    InSessionReposition,
    ReplacementSessionRestart
};

struct MediaPlaybackSeekContract
{
    bool supported = false;
    bool preparing = false;
    MediaPlaybackSeekMode mode = MediaPlaybackSeekMode::Unsupported;
    int windowStartSeconds = 0;
    int windowEndSeconds = 0;
};

struct MediaPlaybackTrackCapabilities
{
    bool audioSelectionSupported = false;
    bool subtitleSelectionSupported = false;
    bool subtitleOffSupported = false;
};

struct MediaPlaybackContract
{
    static constexpr int CurrentVersion = 1;

    int contractVersion = CurrentVersion;
    MediaPlaybackResourceMode resourceMode = MediaPlaybackResourceMode::CompletedRecording;
    std::string presentationProfileId;
    int positionSeconds = 0;
    int durationSeconds = 0;
    int presentationBasePositionSeconds = 0;
    bool pauseSupported = true;
    bool resumePlaybackSupported = true;
    bool restartSupported = false;
    bool restartPreparing = false;
    MediaPlaybackSeekContract seek;
    MediaPlaybackTrackCapabilities tracks;
    int continuityGeneration = 0;
    std::string continuityState = "unpublished";
    std::string failureClass;
    std::string failureReasonCode;
};

class MediaPlaybackContractFactory
{
public:
    static MediaPlaybackContract recording(
        const std::string& presentationProfileId,
        bool growing,
        int positionSeconds,
        int durationSeconds,
        int presentationBasePositionSeconds,
        bool indexedTimelineReady,
        bool indexPreparing,
        const MediaPlaybackTrackCapabilities& tracks = {});

    static MediaPlaybackContract live(
        const std::string& presentationProfileId,
        const MediaPlaybackTrackCapabilities& tracks = {});

    static std::string json(const MediaPlaybackContract& contract);
    static std::string legacyPlaybackJson(const MediaPlaybackContract& contract);
};
