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
        for (std::size_t index = 0; index < size; ++index)
            output[index] = next_++;
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

bool optionBefore(
    const std::vector<std::string>& argv,
    const std::string& first,
    const std::string& second)
{
    const auto firstPosition = std::find(argv.begin(), argv.end(), first);
    const auto secondPosition = std::find(argv.begin(), argv.end(), second);
    return firstPosition != argv.end() && secondPosition != argv.end() &&
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
        ("vdr-suite-recording-seek-activation-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path segment = root / "00001.ts";
    {
        std::ofstream output(segment, std::ios::binary);
        output << "recording";
    }

    int spawnCalls = 0;
    std::vector<std::vector<std::string>> commands;
    RecordingMediaSessionRuntime runtime(
        repository,
        root.string(),
        [&spawnCalls, &commands](
            const std::vector<std::string>& argv,
            const std::string& workingDirectory,
            const std::string&) {
            assert(std::filesystem::is_fifo(
                std::filesystem::path(workingDirectory) / "recording.fmp4"));
            commands.push_back(argv);
            ++spawnCalls;
            return static_cast<pid_t>(5100 + spawnCalls);
        },
        [](pid_t, std::chrono::milliseconds) { return true; },
        [](const std::string&, MediaContainer) { return false; },
        MediaTranscodePolicy{});

    auto issued = issuer.issue(issuanceRequest());
    assert(issued.issued);
    assert(runtime.provisionStream(
        issued.session.sessionId,
        issued.session.workspaceId,
        issued.session.grantId,
        progressiveFmp4Profile(),
        {segment.string()},
        0).ready);
    assert(spawnCalls == 1);

    const std::filesystem::path concatPath =
        root / issued.session.workspaceId / "input.ffconcat";
    const std::string initialConcat = readFile(concatPath);
    assert(initialConcat.find("duration ") == std::string::npos);

    const auto before = runtime.seekStream(issued.session.sessionId, 10);
    assert(!before.repositioned);
    assert(before.reasonCode == "recording_seek_not_supported");
    assert(spawnCalls == 1);

    const auto invalid = runtime.enableIndexedSeek(
        issued.session.sessionId,
        120,
        {segment.string()},
        {});
    assert(!invalid.enabled);
    assert(invalid.reasonCode == "invalid_recording_seek_capability_request");

    const auto enabled = runtime.enableIndexedSeek(
        issued.session.sessionId,
        120,
        {segment.string()},
        {120.0});
    assert(enabled.enabled);
    assert(enabled.durationSeconds == 120);
    assert(spawnCalls == 1);
    const std::string activatedConcat = readFile(concatPath);
    assert(activatedConcat.find("duration 120.000000\n") != std::string::npos);

    const auto idempotent = runtime.enableIndexedSeek(
        issued.session.sessionId,
        120,
        {segment.string()},
        {120.0});
    assert(idempotent.enabled);
    assert(idempotent.durationSeconds == 120);

    const auto seek = runtime.seekStream(issued.session.sessionId, 60);
    assert(seek.repositioned);
    assert(seek.positionSeconds == 60);
    assert(seek.durationSeconds == 120);
    assert(spawnCalls == 2);
    assert(containsPair(commands.back(), "-ss", "60"));
    assert(optionBefore(commands.back(), "-ss", "-i"));
    assert(containsPair(commands.back(), "-i", "input.ffconcat"));

    assert(runtime.stop(issued.session.sessionId, "client_closed"));
    issued.session.clearSecret();
    std::filesystem::remove_all(root);
    return 0;
}
