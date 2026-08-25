#include "RecordingMediaSessionRuntime.h"

#include "Database.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>
#include <vector>

namespace
{

class DeterministicEntropy
{
public:
    bool fill(unsigned char* output, std::size_t size)
    {
        if (output == nullptr || size == 0) return false;
        for (std::size_t index = 0; index < size; ++index) output[index] = next_++;
        return true;
    }
private:
    unsigned char next_ = 1;
};

std::chrono::system_clock::time_point futureClock()
{
    std::tm utc{};
    utc.tm_year = 130;
    utc.tm_mon = 0;
    utc.tm_mday = 1;
    return std::chrono::system_clock::from_time_t(timegm(&utc));
}

MediaSessionIssuanceRequest issuanceRequest()
{
    MediaSessionIssuanceRequest request;
    request.actorId = "actor-1";
    request.backendId = "default";
    request.resourceKind = "recording";
    request.resourceId = "recording-1";
    request.presentationProfileId = "progressive-fmp4";
    request.providerId = "local-vdr-recording";
    return request;
}

MediaPresentationProfile profile(int audioIndex, MediaCodec codec, MediaTrackAction action)
{
    MediaPresentationProfile result;
    result.available = true;
    result.profileId = "progressive-fmp4";
    result.protocol = MediaDeliveryProtocol::Progressive;
    result.container = MediaContainer::Fmp4;
    result.adaptationClass = action == MediaTrackAction::Transcode
        ? MediaAdaptationClass::Transcode
        : MediaAdaptationClass::Remux;
    result.videoAction = MediaTrackAction::Copy;
    result.sourceVideoStreamIndex = 0;
    result.targetVideoCodec = MediaCodec::H264;
    result.targetVideoWidth = 1920;
    result.targetVideoHeight = 1080;
    result.audioAction = action;
    result.sourceAudioStreamIndex = audioIndex;
    result.targetAudioCodec = codec;
    result.targetAudioChannels = 2;
    return result;
}

bool containsPair(
    const std::vector<std::string>& argv,
    const std::string& option,
    const std::string& value)
{
    for (std::size_t index = 0; index + 1 < argv.size(); ++index) {
        if (argv[index] == option && argv[index + 1] == value) return true;
    }
    return false;
}

} // namespace

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MediaSessionRepository repository(database);
    assert(repository.ensureSchema());

    DeterministicEntropy entropy;
    MediaSessionIssuanceService issuer(
        repository,
        [&entropy](unsigned char* output, std::size_t size) {
            return entropy.fill(output, size);
        },
        [] { return futureClock(); });

    const std::filesystem::path root = std::filesystem::temp_directory_path() /
        ("vdr-suite-audio-track-selection-test-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path segment = root / "00001.ts";
    {
        std::ofstream output(segment, std::ios::binary);
        output << "recording";
    }

    int spawnCalls = 0;
    std::vector<pid_t> terminated;
    std::vector<std::vector<std::string>> commands;
    RecordingMediaSessionRuntime runtime(
        repository,
        root.string(),
        [&spawnCalls, &commands](
            const std::vector<std::string>& argv,
            const std::string&,
            const std::string&) {
            commands.push_back(argv);
            ++spawnCalls;
            return static_cast<pid_t>(5100 + spawnCalls);
        },
        [&terminated](pid_t pid, std::chrono::milliseconds) {
            terminated.push_back(pid);
            return true;
        },
        [](const std::string&, MediaContainer) { return false; },
        MediaTranscodePolicy{});

    auto issued = issuer.issue(issuanceRequest());
    assert(issued.issued);
    assert(runtime.provisionStream(
        issued.session.sessionId,
        issued.session.workspaceId,
        issued.session.grantId,
        profile(0, MediaCodec::Aac, MediaTrackAction::Copy),
        {segment.string()},
        3600,
        {3600.0}).ready);
    assert(spawnCalls == 1);
    assert(containsPair(commands.at(0), "-map", "0:a:0?"));

    const auto switched = runtime.selectAudioTrack(
        issued.session.sessionId,
        profile(1, MediaCodec::Aac, MediaTrackAction::Copy),
        125);
    assert(switched.selected);
    assert(switched.restarted);
    assert(switched.reasonCode.empty());
    assert(switched.sourceAudioStreamIndex == 1);
    assert(switched.positionSeconds == 125);
    assert(switched.durationSeconds == 3600);
    assert(spawnCalls == 2);
    assert(terminated.size() == 1);
    assert(terminated.at(0) == 5101);
    assert(containsPair(commands.at(1), "-map", "0:a:1?"));
    assert(containsPair(commands.at(1), "-ss", "125"));

    const auto same = runtime.selectAudioTrack(
        issued.session.sessionId,
        profile(1, MediaCodec::Aac, MediaTrackAction::Copy),
        130);
    assert(same.selected);
    assert(!same.restarted);
    assert(spawnCalls == 2);
    assert(terminated.size() == 1);

    const auto transcoded = runtime.selectAudioTrack(
        issued.session.sessionId,
        profile(2, MediaCodec::Aac, MediaTrackAction::Transcode),
        140);
    assert(transcoded.selected);
    assert(transcoded.restarted);
    assert(transcoded.sourceAudioStreamIndex == 2);
    assert(spawnCalls == 3);
    assert(terminated.size() == 2);
    assert(containsPair(commands.at(2), "-map", "0:a:2?"));
    assert(containsPair(commands.at(2), "-c:a", "aac"));
    assert(containsPair(commands.at(2), "-ss", "140"));

    const auto outside = runtime.selectAudioTrack(
        issued.session.sessionId,
        profile(0, MediaCodec::Aac, MediaTrackAction::Copy),
        3600);
    assert(!outside.selected);
    assert(outside.reasonCode == "recording_audio_track_position_outside_window");
    assert(spawnCalls == 3);
    assert(terminated.size() == 2);

    assert(runtime.stop(issued.session.sessionId, "client_closed"));
    assert(terminated.size() == 3);
    assert(terminated.at(2) == 5103);
    issued.session.clearSecret();

    auto noTimeline = issuer.issue(issuanceRequest());
    assert(noTimeline.issued);
    assert(runtime.provisionStream(
        noTimeline.session.sessionId,
        noTimeline.session.workspaceId,
        noTimeline.session.grantId,
        profile(0, MediaCodec::Aac, MediaTrackAction::Copy),
        {segment.string()},
        0).ready);
    const auto unsupported = runtime.selectAudioTrack(
        noTimeline.session.sessionId,
        profile(1, MediaCodec::Aac, MediaTrackAction::Copy),
        10);
    assert(!unsupported.selected);
    assert(unsupported.reasonCode == "recording_audio_track_selection_not_supported");
    assert(runtime.stop(noTimeline.session.sessionId, "client_closed"));
    noTimeline.session.clearSecret();

    std::filesystem::remove_all(root);
    return 0;
}
