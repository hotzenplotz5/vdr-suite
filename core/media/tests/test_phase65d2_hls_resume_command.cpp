#include "FfmpegHlsCommandBuilder.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

namespace
{

std::size_t indexOf(
    const std::vector<std::string>& argv,
    const std::string& value)
{
    const auto found = std::find(argv.begin(), argv.end(), value);
    return found == argv.end()
        ? argv.size()
        : static_cast<std::size_t>(std::distance(argv.begin(), found));
}

MediaPresentationProfile hlsProfile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "hls-fmp4";
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Transcode;
    profile.videoAction = MediaTrackAction::Transcode;
    profile.audioAction = MediaTrackAction::Transcode;
    profile.sourceVideoStreamIndex = 0;
    profile.sourceAudioStreamIndex = 0;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetAudioCodec = MediaCodec::Aac;
    profile.targetVideoWidth = 1920;
    profile.targetVideoHeight = 1080;
    profile.targetAudioChannels = 2;
    return profile;
}

} // namespace

int main()
{
    const FfmpegHlsCommandBuilder builder;

    const auto ordinary = builder.build(hlsProfile());
    assert(ordinary.valid);
    assert(indexOf(ordinary.argv, "-ss") == ordinary.argv.size());

    const auto resumed = builder.build(hlsProfile(), 2494);
    assert(resumed.valid);
    const std::size_t seek = indexOf(resumed.argv, "-ss");
    const std::size_t input = indexOf(resumed.argv, "-i");
    assert(seek < resumed.argv.size());
    assert(seek + 1 < resumed.argv.size());
    assert(resumed.argv[seek + 1] == "2494");
    assert(seek < input);
    assert(input + 1 < resumed.argv.size());
    assert(resumed.argv[input + 1] == "input.ffconcat");

    const auto invalid = builder.build(hlsProfile(), -1);
    assert(!invalid.valid);
    assert(invalid.reasonCode == "invalid_recording_start_position");

    return 0;
}
