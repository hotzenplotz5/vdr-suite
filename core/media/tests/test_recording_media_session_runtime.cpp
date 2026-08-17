#include "RecordingMediaSessionRuntime.h"

#include "Database.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <string>
#include <unistd.h>

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
    request.presentationProfileId = "hls-fmp4";
    request.providerId = "local-vdr-recording";
    return request;
}

MediaPresentationProfile hlsProfile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "hls-fmp4";
    profile.protocol = MediaDeliveryProtocol::Hls;
    profile.container = MediaContainer::Fmp4;
    profile.videoAction = MediaTrackAction::Omit;
    profile.audioAction = MediaTrackAction::Omit;
    return profile;
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

    auto issued = issuer.issue(issuanceRequest());
    assert(issued.issued);

    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-runtime-test-" + std::to_string(::getpid()));
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root);
    const std::filesystem::path segment = root / "00001.ts";
    {
        std::ofstream output(segment, std::ios::binary);
        output << "recording";
    }

    int spawnCalls = 0;
    int terminateCalls = 0;
    {
        RecordingMediaSessionRuntime runtime(
            repository,
            root.string(),
            [&spawnCalls](const std::vector<std::string>& argv,
                          const std::string& workingDirectory,
                          const std::string& logPath) {
                assert(!argv.empty());
                assert(argv.front() == "/usr/bin/ffmpeg");
                assert(!workingDirectory.empty());
                assert(!logPath.empty());
                ++spawnCalls;
                return static_cast<pid_t>(4242);
            },
            [&terminateCalls](pid_t pid, std::chrono::milliseconds grace) {
                assert(pid == 4242);
                assert(grace.count() >= 0);
                ++terminateCalls;
                return true;
            },
            [](const std::string& workingDirectory, MediaContainer container) {
                assert(!workingDirectory.empty());
                return container == MediaContainer::Fmp4;
            });

        const auto result = runtime.provisionHls(
            issued.session.sessionId,
            issued.session.workspaceId,
            hlsProfile(),
            {segment.string()});
        assert(result.ready);
        assert(result.reasonCode.empty());
        assert(spawnCalls == 1);

        const auto ready = repository.findSession(issued.session.sessionId);
        assert(ready.has_value());
        assert(ready->state == "ready");

        assert(runtime.stop(issued.session.sessionId, "client_closed"));
        assert(!runtime.stop(issued.session.sessionId, "client_closed"));
        assert(terminateCalls == 1);

        const auto stopped = repository.findSession(issued.session.sessionId);
        assert(stopped.has_value());
        assert(stopped->state == "ended");
        assert(stopped->terminalReason == "client_closed");
        assert(!std::filesystem::exists(root / issued.session.workspaceId));
    }

    assert(terminateCalls == 1);
    const auto ended = repository.findSession(issued.session.sessionId);
    assert(ended.has_value());
    assert(ended->state == "ended");
    assert(ended->terminalReason == "client_closed");

    issued.session.clearSecret();
    std::filesystem::remove_all(root);
    return 0;
}