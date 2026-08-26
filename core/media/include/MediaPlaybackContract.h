#pragma once

#include <optional>
#include <string>

enum class MediaPlaybackResourceMode
{
    Recording,
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
    std::optional<bool> supported;
    std::optional<bool> preparing;
    MediaPlaybackSeekMode mode = MediaPlaybackSeekMode::Unsupported;
    std::optional<int> windowStartSeconds;
    std::optional<int> windowEndSeconds;
};

struct MediaPlaybackTrackCapabilities
{
    std::optional<bool> audioSelectionSupported;
    std::optional<bool> subtitleSelectionSupported;
    std::optional<bool> subtitleOffSupported;
};

struct MediaPlaybackContract
{
    static constexpr int CurrentVersion = 1;

    int contractVersion = CurrentVersion;
    MediaPlaybackResourceMode resourceMode = MediaPlaybackResourceMode::Recording;
    std::string presentationProfileId;
    std::optional<int> positionSeconds;
    std::optional<int> durationSeconds;
    std::optional<int> presentationBasePositionSeconds;
    std::optional<bool> pauseSupported;
    std::optional<bool> resumePlaybackSupported;
    std::optional<bool> restartSupported;
    std::optional<bool> restartPreparing;
    MediaPlaybackSeekContract seek;
    MediaPlaybackTrackCapabilities tracks;
    std::optional<int> continuityGeneration;
    std::optional<std::string> continuityState;
    std::optional<std::string> failureClass;
    std::optional<std::string> failureReasonCode;
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

    static MediaPlaybackContract recordingFromLegacy(
        const std::string& presentationProfileId,
        std::optional<bool> growing,
        std::optional<int> positionSeconds,
        std::optional<int> durationSeconds,
        std::optional<int> presentationBasePositionSeconds,
        std::optional<bool> legacySeekSupported,
        std::optional<bool> legacySeekPreparing,
        std::optional<bool> restartSupported,
        std::optional<bool> restartPreparing,
        const MediaPlaybackTrackCapabilities& tracks = {});

    static MediaPlaybackContract live(
        const std::string& presentationProfileId,
        const MediaPlaybackTrackCapabilities& tracks = {});

    static std::string json(const MediaPlaybackContract& contract);
    static std::string legacyPlaybackJson(const MediaPlaybackContract& contract);
};
