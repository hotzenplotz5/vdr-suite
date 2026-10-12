#include "MediaEmittedFmp4Probe.h"
#include "MediaPresentationSelector.h"
#include "MediaPlannedOutput.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

namespace
{

MediaPlannedOutput expectedFmp4()
{
    MediaSourceDescriptor source;
    source.container = MediaContainer::MpegTs;
    source.videoStreams.push_back({
        MediaCodec::H264, 320, 180, 25.0, false
    });
    source.audioStreams.push_back({MediaCodec::Aac, 2, "deu"});

    ClientMediaCapabilities caps;
    caps.protocols = {MediaDeliveryProtocol::Hls};
    caps.containers = {MediaContainer::Fmp4};
    caps.videoCodecs = {MediaCodec::H264};
    caps.audioCodecs = {MediaCodec::Aac};
    caps.maxVideoWidth = 320;
    caps.maxVideoHeight = 180;
    caps.maxAudioChannels = 2;

    const auto selected = MediaPresentationSelector().select(source, caps);
    assert(selected.available);
    assert(selected.profileId == "hls-fmp4");
    assert(selected.videoAction == MediaTrackAction::Copy);
    assert(selected.audioAction == MediaTrackAction::Copy);
    return projectPlannedOutput(source, selected);
}

} // namespace

int main(int argc, char** argv)
{
    const MediaEmittedFmp4Probe probe;
    MediaPlannedOutput plan = expectedFmp4();

    const auto command = probe.commandPlan();
    assert(command.front() == "/usr/bin/ffprobe");
    assert(command.back() == "init.mp4");
    for (const auto& argument : command) {
        assert(argument.find("http:") == std::string::npos);
        assert(argument.find("ffconcat") == std::string::npos);
    }

    const std::string valid =
        "codec_name=h264|codec_type=video|codec_tag_string=avc1|width=320|height=180\n"
        "codec_name=aac|codec_type=audio|codec_tag_string=mp4a|channels=2\n";
    const auto verified = probe.parse(valid, plan);
    assert(verified.inspected && verified.matchesPlannedOutput);
    assert(verified.videoCodec == MediaCodec::H264);
    assert(verified.audioCodec == MediaCodec::Aac);
    assert(verified.videoWidth == 320 && verified.videoHeight == 180);
    assert(verified.audioChannels == 2);

    const std::string h265 =
        "codec_name=hevc|codec_type=video|codec_tag_string=hvc1|width=320|height=180\n"
        "codec_name=aac|codec_type=audio|codec_tag_string=mp4a|channels=2\n";
    auto mismatch = probe.parse(h265, plan);
    assert(mismatch.inspected && !mismatch.matchesPlannedOutput);
    assert(mismatch.reasonCode == "emitted_init_codec_mismatch");

    const std::string ac3 =
        "codec_name=h264|codec_type=video|codec_tag_string=avc1|width=320|height=180\n"
        "codec_name=ac3|codec_type=audio|codec_tag_string=ac-3|channels=2\n";
    mismatch = probe.parse(ac3, plan);
    assert(mismatch.inspected && !mismatch.matchesPlannedOutput);
    assert(mismatch.audioCodec == MediaCodec::Ac3);
    assert(!probe.parse(valid + valid, plan).inspected);
    assert(!probe.parse(valid + "codec_name=weird|codec_type=subtitle|codec_tag_string=tx3g\n", plan).inspected);
    assert(!probe.parse(
        "codec_name=h264|codec_type=video|codec_tag_string=mp4a|width=320|height=180\n"
        "codec_name=aac|codec_type=audio|codec_tag_string=mp4a|channels=2\n", plan).inspected);
    assert(!probe.parse(
        "codec_name=h264|codec_type=video|codec_tag_string=avc1|width=320|height=180\n", plan).inspected);
    assert(!probe.parse(std::string(9000, 'x'), plan).inspected);
    MediaPlannedOutput invalid;
    assert(!probe.parse(valid, invalid).inspected);
    assert(!probe.inspectOwnedWorkspace("/tmp", plan).inspected);
    assert(!probe.inspectOwnedWorkspace("../../tmp", plan).inspected);

    if (argc == 2) {
        const std::string directory(argv[1]);
        const auto fromRealInit = probe.inspectOwnedWorkspace(directory, plan);
        assert(fromRealInit.inspected);
        assert(fromRealInit.matchesPlannedOutput);
        assert(fromRealInit.videoCodec == MediaCodec::H264);
        assert(fromRealInit.audioCodec == MediaCodec::Aac);
        assert(fromRealInit.videoWidth == 320);
        assert(fromRealInit.videoHeight == 180);
        assert(fromRealInit.audioChannels == 2);

        // The emitted file remains exactly where FFmpeg produced it.
        assert(std::filesystem::is_regular_file(
            std::filesystem::path(directory) / "init.mp4"));
        const auto manifest = std::filesystem::path(directory) / "master.m3u8";
        assert(std::filesystem::is_regular_file(manifest));

        const std::filesystem::path output = std::filesystem::path(directory) / "init.mp4";
        const std::filesystem::path moved = std::filesystem::path(directory) / "real-init.mp4";
        std::filesystem::rename(output, moved);
        std::filesystem::create_symlink(moved, output);
        const auto rejected = probe.inspectOwnedWorkspace(directory, plan);
        assert(!rejected.inspected);
        assert(rejected.reasonCode == "emitted_init_unavailable");
        std::filesystem::remove(output);
        std::filesystem::rename(moved, output);

        std::ofstream broken(output, std::ios::binary | std::ios::trunc);
        broken << "not-an-fmp4-file";
        broken.close();
        const auto malformed = probe.inspectOwnedWorkspace(directory, plan);
        assert(!malformed.inspected);
        assert(malformed.reasonCode == "emitted_probe_process_failed");
    }
    return 0;
}
