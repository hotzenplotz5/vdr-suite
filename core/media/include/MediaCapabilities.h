#pragma once

#include <string>
#include <vector>

enum class MediaResourceKind
{
    Recording,
    LiveChannel,
    GrowingRecording
};

enum class MediaDeliveryProtocol
{
    Progressive,
    Hls
};

enum class MediaContainer
{
    Unknown,
    MpegTs,
    Mp4,
    Fmp4
};

enum class MediaCodec
{
    None,
    Unknown,
    H264,
    H265,
    Mpeg2Video,
    Aac,
    Ac3,
    Eac3,
    MpegAudio
};

enum class MediaAdaptationClass
{
    PassThrough,
    Remux,
    Transcode
};

enum class MediaTrackAction
{
    Copy,
    Transcode,
    Omit
};

struct MediaVideoStreamDescriptor
{
    MediaCodec codec = MediaCodec::Unknown;
    int width = 0;
    int height = 0;
    double framesPerSecond = 0.0;
    bool hdr = false;
};

struct MediaAudioStreamDescriptor
{
    MediaCodec codec = MediaCodec::Unknown;
    int channels = 0;
    std::string language;
};

struct MediaSourceDescriptor
{
    MediaResourceKind resourceKind = MediaResourceKind::Recording;
    MediaContainer container = MediaContainer::Unknown;
    std::vector<MediaVideoStreamDescriptor> videoStreams;
    std::vector<MediaAudioStreamDescriptor> audioStreams;
    bool seekable = false;
    bool growing = false;
};

struct ClientMediaCapabilities
{
    std::vector<MediaDeliveryProtocol> protocols;
    std::vector<MediaContainer> containers;
    std::vector<MediaCodec> videoCodecs;
    std::vector<MediaCodec> audioCodecs;
    bool supportsByteRanges = false;
    int maxVideoWidth = 0;
    int maxVideoHeight = 0;
    int maxAudioChannels = 0;
};

struct MediaPresentationProfile
{
    bool available = false;
    std::string profileId;
    MediaDeliveryProtocol protocol = MediaDeliveryProtocol::Progressive;
    MediaContainer container = MediaContainer::Unknown;
    MediaAdaptationClass adaptationClass = MediaAdaptationClass::PassThrough;
    MediaTrackAction videoAction = MediaTrackAction::Copy;
    MediaTrackAction audioAction = MediaTrackAction::Copy;
    int sourceVideoStreamIndex = -1;
    int sourceAudioStreamIndex = -1;
    MediaCodec targetVideoCodec = MediaCodec::None;
    MediaCodec targetAudioCodec = MediaCodec::None;
    int targetVideoWidth = 0;
    int targetVideoHeight = 0;
    int targetAudioChannels = 0;
    std::string reason;
};
