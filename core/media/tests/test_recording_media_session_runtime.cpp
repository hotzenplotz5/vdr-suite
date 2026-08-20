#include "RecordingMediaSessionRuntime.h"

#include "Database.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "RecordingSourceFingerprint.h"

#include <algorithm>
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

MediaSessionIssuanceRequest issuanceRequest(
    const std::string& profileId = "hls-fmp4")
{
    MediaSessionIssuanceRequest request;
    request.actorId = "actor-1";
    request.backendId = "default";
    request.resourceKind = "recording";
    request.resourceId = "recording-1";
    request.presentationProfileId = profileId;
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
    profile.videoAction = MediaTrackAction::Transcode;
    profile.sourceVideoStreamIndex = 0;
    profile.targetVideoCodec = MediaCodec::H264;
    profile.targetVideoWidth = 1920;
    profile.targetVideoHeight = 1080;
    profile.deinterlaceVideo = true;
    profile.videoTranscodeWorkload = MediaTranscodeWorkload::Deinterlace;
    profile.audioAction = MediaTrackAction::Omit;
    return profile;
}

MediaPresentationProfile directProfile()
{
    MediaPresentationProfile profile;
    profile.available = true;
    profile.profileId = "progressive-direct";
    profile.protocol = MediaDeliveryProtocol::Progressive;
    profile.container = MediaContainer::MpegTs;
    profile.adaptationClass = MediaAdaptationClass::PassThrough;
    profile.videoAction = MediaTrackAction::Copy;
    profile.audioAction = MediaTrackAction::Copy;
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

    MediaTranscodePerformanceSamples samples;
    samples[MediaTranscodeWorkload::Deinterlace][
        MediaSoftwareEncoderPreset::Veryfast] = 0.992;
    samples[MediaTranscodeWorkload::Deinterlace][
        MediaSoftwareEncoderPreset::Superfast] = 1.54;
    MediaTranscodePolicy transcodePolicy(MediaTranscodePolicyConfig{}, samples);

    int spawnCalls = 0;
    int terminateCalls = 0;
    std::string idleSessionId;
    {
        RecordingMediaSessionRuntime runtime(
            repository,
            root.string(),
            [&spawnCalls](const std::vector<std::string>& argv,
                          const std::string& workingDirectory,
                          const std::string& logPath) {
                assert(!argv.empty());
                assert(argv.front() == "/usr/bin/ffmpeg");
                assert(containsPair(argv, "-c:v", "libx264"));
                assert(containsPair(argv, "-preset", "superfast"));
                assert(containsPair(
                    argv,
                    "-vf",
                    "bwdif=mode=send_frame:parity=auto:deint=all,scale=1920:1080"));
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
            },
            transcodePolicy);

        const auto result = runtime.provisionHls(
            issued.session.sessionId,
            issued.session.workspaceId,
            issued.session.grantId,
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

        auto idleIssued = issuer.issue(issuanceRequest());
        assert(idleIssued.issued);
        idleSessionId = idleIssued.session.sessionId;

        const auto idleResult = runtime.provisionHls(
            idleIssued.session.sessionId,
            idleIssued.session.workspaceId,
            idleIssued.session.grantId,
            hlsProfile(),
            {segment.string()});
        assert(idleResult.ready);
        assert(spawnCalls == 2);

        assert(runtime.reapInactive(300) == 0);
        assert(terminateCalls == 1);
        const auto stillReady = repository.findSession(idleIssued.session.sessionId);
        assert(stillReady.has_value());
        assert(stillReady->state == "ready");

        assert(database.execute(
            "UPDATE media_access_grants SET "
            "last_seen_at=datetime('now','-301 seconds'), "
            "updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id='" + idleIssued.session.sessionId + "';"));

        const auto idleGrant = repository.findResolvedGrant(
            idleIssued.session.grantId,
            300);
        assert(idleGrant.has_value());
        assert(idleGrant->active);
        assert(idleGrant->idleExpired);

        assert(runtime.reapInactive(300) == 1);
        assert(runtime.reapInactive(300) == 0);
        assert(terminateCalls == 2);

        const auto reaped = repository.findSession(idleIssued.session.sessionId);
        assert(reaped.has_value());
        assert(reaped->state == "ended");
        assert(reaped->terminalReason == "media_access_idle_expired");
        assert(!std::filesystem::exists(root / idleIssued.session.workspaceId));

        idleIssued.session.clearSecret();
    }

    assert(terminateCalls == 2);
    const auto ended = repository.findSession(issued.session.sessionId);
    assert(ended.has_value());
    assert(ended->state == "ended");
    assert(ended->terminalReason == "client_closed");

    const auto idleEnded = repository.findSession(idleSessionId);
    assert(idleEnded.has_value());
    assert(idleEnded->state == "ended");
    assert(idleEnded->terminalReason == "media_access_idle_expired");

    const std::vector<std::string> directSegments = {segment.string()};
    const RecordingSourceFingerprint fingerprint =
        inspectRecordingSource(root.string(), directSegments);
    assert(fingerprint.valid);
    RecordingDirectSourceRegistration registration;
    registration.recordingDirectory = root.string();
    registration.segmentPaths = directSegments;
    registration.sourceFingerprint = fingerprint.value;
    registration.readableBytes = fingerprint.readableBytes;

    RecordingDirectSourceRegistry directRegistry;
    {
        RecordingMediaSessionRuntime directRuntime(
            repository,
            root.string(),
            directRegistry);

        auto directIssued = issuer.issue(issuanceRequest("progressive-direct"));
        assert(directIssued.issued);
        const auto directResult = directRuntime.provisionDirect(
            directIssued.session.sessionId,
            directIssued.session.grantId,
            directProfile(),
            registration);
        assert(directResult.ready);
        assert(directRegistry.lookup(directIssued.session.sessionId).available);
        assert(directRuntime.stop(directIssued.session.sessionId, "client_closed"));
        assert(!directRegistry.lookup(directIssued.session.sessionId).available);
        const auto directStopped = repository.findSession(directIssued.session.sessionId);
        assert(directStopped.has_value());
        assert(directStopped->state == "ended");
        assert(directStopped->terminalReason == "client_closed");
        directIssued.session.clearSecret();

        auto directIdle = issuer.issue(issuanceRequest("progressive-direct"));
        assert(directIdle.issued);
        assert(directRuntime.provisionDirect(
            directIdle.session.sessionId,
            directIdle.session.grantId,
            directProfile(),
            registration).ready);
        assert(directRegistry.lookup(directIdle.session.sessionId).available);
        assert(database.execute(
            "UPDATE media_access_grants SET "
            "last_seen_at=datetime('now','-301 seconds'), "
            "updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id='" + directIdle.session.sessionId + "';"));
        assert(directRuntime.reapInactive(300) == 1);
        assert(!directRegistry.lookup(directIdle.session.sessionId).available);
        const auto directReaped = repository.findSession(directIdle.session.sessionId);
        assert(directReaped.has_value());
        assert(directReaped->state == "ended");
        assert(directReaped->terminalReason == "media_access_idle_expired");
        directIdle.session.clearSecret();
    }

    issued.session.clearSecret();
    std::filesystem::remove_all(root);
    return 0;
}
