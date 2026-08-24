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
#include <iterator>
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
        for (std::size_t index = 0; index < size; ++index) {
            output[index] = next_++;
        }
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

MediaPresentationProfile progressiveFmp4Profile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "progressive-fmp4";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = MediaContainer::Fmp4;
    profile.adaptationClass = MediaAdaptationClass::Remux;
    profile.videoAction = MediaTrackAction::Copy;
    profile.sourceVideoStreamIndex = 0;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.audioAction = MediaTrackAction::Copy;
    profile.sourceAudioStreamIndex = 0;
    profile.targetAudioCodec = MediaCodec::Aac;
    profile.targetAudioChannels = 2;
    return profile;
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

bool containsValue(
    const std::vector<std::string>& argv,
    const std::string& value)
{
    return std::find(argv.begin(), argv.end(), value) != argv.end();
}

bool optionBefore(
    const std::vector<std::string>& argv,
    const std::string& first,
    const std::string& second)
{
    const auto firstPosition = std::find(argv.begin(), argv.end(), first);
    const auto secondPosition = std::find(argv.begin(), argv.end(), second);
    return firstPosition != argv.end() &&
        secondPosition != argv.end() &&
        firstPosition < secondPosition;
}

std::string readFile(const std::filesystem::path& path)
{
    std::ifstream input(path);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
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

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-recording-seek-test-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path segment = root / "00001.ts";
    {
        std::ofstream output(segment, std::ios::binary);
        output << "recording";
    }

    int spawnCalls = 0;
    std::vector<pid_t> terminatedPids;
    std::vector<std::vector<std::string>> commands;
    RecordingMediaSessionRuntime runtime(
        repository,
        root.string(),
        [&spawnCalls, &commands](
            const std::vector<std::string>& argv,
            const std::string& workingDirectory,
            const std::string& logPath) {
            assert(!argv.empty());
            assert(argv.front() == "/usr/bin/ffmpeg");
            assert(containsPair(argv, "-f", "concat"));
            assert(containsPair(argv, "-i", "input.ffconcat"));
            assert(argv.back() == "recording.fmp4");
            assert(std::filesystem::is_fifo(
                std::filesystem::path(workingDirectory) / "recording.fmp4"));
            assert(!logPath.empty());
            commands.push_back(argv);
            ++spawnCalls;
            return static_cast<pid_t>(4100 + spawnCalls);
        },
        [&terminatedPids](pid_t pid, std::chrono::milliseconds grace) {
            assert(pid > 0);
            assert(grace.count() >= 0);
            terminatedPids.push_back(pid);
            return true;
        },
        [](const std::string&, MediaContainer) { return false; },
        MediaTranscodePolicy{});

    auto issued = issuer.issue(issuanceRequest());
    assert(issued.issued);
    const auto provisioned = runtime.provisionStream(
        issued.session.sessionId,
        issued.session.workspaceId,
        issued.session.grantId,
        progressiveFmp4Profile(),
        {segment.string()},
        5400,
        {5400.0});
    assert(provisioned.ready);
    assert(spawnCalls == 1);
    assert(!containsValue(commands.at(0), "-ss"));
    assert(std::filesystem::is_fifo(
        root / issued.session.workspaceId / "recording.fmp4"));
    const std::string concat =
        readFile(root / issued.session.workspaceId / "input.ffconcat");
    assert(concat.find("file 'source-000001.ts'\n") != std::string::npos);
    assert(concat.find("duration 5400.000000\n") != std::string::npos);

    const auto outside = runtime.seekStream(issued.session.sessionId, 5400);
    assert(!outside.repositioned);
    assert(outside.reasonCode == "recording_seek_outside_window");
    assert(outside.durationSeconds == 5400);
    assert(spawnCalls == 1);
    assert(terminatedPids.empty());

    const auto negative = runtime.seekStream(issued.session.sessionId, -1);
    assert(!negative.repositioned);
    assert(negative.reasonCode == "invalid_recording_seek_request");
    assert(spawnCalls == 1);
    assert(terminatedPids.empty());

    const auto seek = runtime.seekStream(issued.session.sessionId, 2530);
    assert(seek.repositioned);
    assert(seek.reasonCode.empty());
    assert(seek.positionSeconds == 2530);
    assert(seek.durationSeconds == 5400);
    assert(spawnCalls == 2);
    assert(terminatedPids.size() == 1);
    assert(terminatedPids.at(0) == 4101);
    assert(containsPair(commands.at(1), "-ss", "2530"));
    assert(optionBefore(commands.at(1), "-ss", "-i"));
    assert(std::filesystem::is_fifo(
        root / issued.session.workspaceId / "recording.fmp4"));
    const auto stillReady = repository.findSession(issued.session.sessionId);
    assert(stillReady.has_value());
    assert(stillReady->state == "ready");

    const auto rewindToStart = runtime.seekStream(issued.session.sessionId, 0);
    assert(rewindToStart.repositioned);
    assert(rewindToStart.positionSeconds == 0);
    assert(rewindToStart.durationSeconds == 5400);
    assert(spawnCalls == 3);
    assert(terminatedPids.size() == 2);
    assert(terminatedPids.at(1) == 4102);
    assert(!containsValue(commands.at(2), "-ss"));

    assert(runtime.stop(issued.session.sessionId, "client_closed"));
    assert(terminatedPids.size() == 3);
    assert(terminatedPids.at(2) == 4103);
    const auto stopped = repository.findSession(issued.session.sessionId);
    assert(stopped.has_value());
    assert(stopped->state == "ended");
    assert(stopped->terminalReason == "client_closed");
    issued.session.clearSecret();

    auto unknownDuration = issuer.issue(issuanceRequest());
    assert(unknownDuration.issued);
    assert(runtime.provisionStream(
        unknownDuration.session.sessionId,
        unknownDuration.session.workspaceId,
        unknownDuration.session.grantId,
        progressiveFmp4Profile(),
        {segment.string()},
        0).ready);
    assert(spawnCalls == 4);

    const auto unsupported = runtime.seekStream(
        unknownDuration.session.sessionId,
        10);
    assert(!unsupported.repositioned);
    assert(unsupported.reasonCode == "recording_seek_not_supported");
    assert(spawnCalls == 4);
    assert(terminatedPids.size() == 3);

    assert(runtime.stop(unknownDuration.session.sessionId, "client_closed"));
    assert(terminatedPids.size() == 4);
    assert(terminatedPids.at(3) == 4104);
    unknownDuration.session.clearSecret();

    auto durationWithoutTimeline = issuer.issue(issuanceRequest());
    assert(durationWithoutTimeline.issued);
    assert(runtime.provisionStream(
        durationWithoutTimeline.session.sessionId,
        durationWithoutTimeline.session.workspaceId,
        durationWithoutTimeline.session.grantId,
        progressiveFmp4Profile(),
        {segment.string()},
        5400).ready);
    assert(spawnCalls == 5);
    const auto noTimelineSeek = runtime.seekStream(
        durationWithoutTimeline.session.sessionId,
        10);
    assert(!noTimelineSeek.repositioned);
    assert(noTimelineSeek.reasonCode == "recording_seek_not_supported");
    assert(spawnCalls == 5);
    assert(runtime.stop(
        durationWithoutTimeline.session.sessionId,
        "client_closed"));
    assert(terminatedPids.size() == 5);
    assert(terminatedPids.at(4) == 4105);
    durationWithoutTimeline.session.clearSecret();

    std::filesystem::remove_all(root);
    return 0;
}
