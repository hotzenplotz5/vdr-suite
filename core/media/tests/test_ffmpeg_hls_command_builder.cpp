#include "FfmpegHlsCommandBuilder.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

namespace
{

bool containsPair(
    const std::vector<std::string>& argv,
    const std::string& option,
    const std::string& value)
{
    for (std::size_t index = 0; index + 1 < argv.size(); ++index) {
        if (argv[index] == option && argv[index + 1] == value) {
            return true;
        }
    }
    return false;
}

bool containsValue(
    const std::vector<std::string>& argv,
    const std::string& value)
{
    return std::find(argv.begin(), argv.end(), value) != argv.end();
}

MediaPresentationProfile hlsFmp4Profile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "untrusted-profile-name;rm -rf /";
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Transcode;
    profile.videoAction = MediaTrackAction::Copy;
    profile.audioAction = MediaTrackAction::Transcode;
    profile.sourceVideoStreamIndex = 0;
    profile.sourceAudioStreamIndex = 1;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetAudioCodec = MediaCodec::Aac;
    profile.targetAudioChannels = 2;
    return profile;
}

} // namespace

int main()
{
    FfmpegHlsCommandBuilder builder;

    {
        const auto plan = builder.build(hlsFmp4Profile());
        assert(plan.valid);
        assert(plan.reasonCode.empty());
        assert(!plan.argv.empty());
        assert(plan.argv.front() == "/usr/bin/ffmpeg");
        assert(plan.argv.back() == "master.m3u8");
        assert(!containsValue(plan.argv, "/bin/sh"));
        assert(containsValue(plan.argv, "-re"));
        assert(containsPair(plan.argv, "-map", "0:v:0?"));
        assert(containsPair(plan.argv, "-map", "0:a:1?"));
        assert(containsPair(plan.argv, "-c:v", "copy"));
        assert(containsPair(plan.argv, "-c:a", "aac"));
        assert(containsPair(plan.argv, "-b:a", "192k"));
        assert(containsPair(plan.argv, "-ac", "2"));
        assert(containsPair(plan.argv, "-bsf:v", "h264_metadata=level=auto"));
        assert(!containsPair(plan.argv, "-bsf:a", "aac_adtstoasc"));
        assert(containsPair(plan.argv, "-hls_time", "4"));
        assert(containsPair(plan.argv, "-hls_list_size", "8"));
        assert(containsPair(plan.argv, "-hls_delete_threshold", "2"));
        assert(containsPair(
            plan.argv,
            "-hls_flags",
            "delete_segments+independent_segments+temp_file"));
        assert(!containsValue(plan.argv, "-hls_playlist_type"));
        assert(!containsValue(plan.argv, "event"));
        assert(containsPair(plan.argv, "-hls_segment_type", "fmp4"));
        assert(containsPair(plan.argv, "-hls_fmp4_init_filename", "init.mp4"));
        assert(containsPair(plan.argv, "-hls_segment_filename", "segment-%06d.m4s"));
        assert(!containsValue(plan.argv, "untrusted-profile-name;rm -rf /"));
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.adaptationClass = MediaAdaptationClass::Remux;
        profile.audioAction = MediaTrackAction::Copy;
        profile.targetAudioCodec = MediaCodec::Aac;

        const auto plan = builder.build(profile);
        assert(plan.valid);
        assert(containsPair(plan.argv, "-c:v", "copy"));
        assert(containsPair(plan.argv, "-c:a", "copy"));
        assert(!containsValue(plan.argv, "-ac"));
        assert(containsPair(plan.argv, "-bsf:v", "h264_metadata=level=auto"));
        assert(containsPair(plan.argv, "-bsf:a", "aac_adtstoasc"));
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.container = MediaContainer::MpegTs;
        profile.adaptationClass = MediaAdaptationClass::Remux;
        profile.audioAction = MediaTrackAction::Copy;
        profile.targetAudioCodec = MediaCodec::Aac;

        const auto plan = builder.build(profile);
        assert(plan.valid);
        assert(containsPair(plan.argv, "-c:v", "copy"));
        assert(containsPair(plan.argv, "-c:a", "copy"));
        assert(!containsValue(plan.argv, "-ac"));
        assert(!containsPair(plan.argv, "-bsf:v", "h264_metadata=level=auto"));
        assert(!containsPair(plan.argv, "-bsf:a", "aac_adtstoasc"));
        assert(containsPair(plan.argv, "-hls_segment_filename", "segment-%06d.ts"));
        assert(!containsValue(plan.argv, "-hls_segment_type"));
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.targetAudioChannels = 6;

        const auto plan = builder.build(profile);
        assert(plan.valid);
        assert(containsPair(plan.argv, "-ac", "6"));
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.targetAudioChannels = 33;

        const auto plan = builder.build(profile);
        assert(!plan.valid);
        assert(plan.reasonCode == "unsupported_track_transformation");
        assert(plan.argv.empty());
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.videoAction = MediaTrackAction::Transcode;
        profile.targetVideoCodec = MediaCodec::H264;

        const auto plan = builder.build(profile);
        assert(plan.valid);
        assert(containsPair(plan.argv, "-c:v", "libx264"));
        assert(containsPair(plan.argv, "-preset", "veryfast"));
        assert(containsPair(plan.argv, "-crf", "20"));
        assert(!containsPair(plan.argv, "-bsf:v", "h264_metadata=level=auto"));
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.sourceAudioStreamIndex = -1;

        const auto plan = builder.build(profile);
        assert(!plan.valid);
        assert(plan.reasonCode == "selected_source_track_missing");
        assert(plan.argv.empty());
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.videoAction = MediaTrackAction::Transcode;
        profile.targetVideoCodec = MediaCodec::H265;

        const auto plan = builder.build(profile);
        assert(!plan.valid);
        assert(plan.reasonCode == "unsupported_track_transformation");
        assert(plan.argv.empty());
    }

    {
        MediaPresentationProfile profile = hlsFmp4Profile();
        profile.protocol = MediaDeliveryProtocol::Progressive;

        const auto plan = builder.build(profile);
        assert(!plan.valid);
        assert(plan.reasonCode == "profile_is_not_hls");
    }

    return 0;
}
