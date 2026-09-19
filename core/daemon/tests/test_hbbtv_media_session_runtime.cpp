#include "Database.h"
#include "HbbtvMediaSessionRuntime.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace
{
class Entropy
{
public:
    bool fill(
        unsigned char* output,
        std::size_t size)
    {
        if (output == nullptr)
            return false;
        for (std::size_t index = 0;
             index < size;
             ++index)
        {
            output[index] =
                static_cast<unsigned char>(
                    (counter_++ % 251U) + 1U);
        }
        return true;
    }

private:
    unsigned int counter_ = 1;
};

pid_t spawnIdleWorker()
{
    const pid_t pid = ::fork();
    if (pid == 0)
        for (;;) ::pause();
    return pid;
}

bool terminateWorker(pid_t pid)
{
    if (pid <= 0)
        return false;
    if (::kill(pid, SIGTERM) != 0)
        return false;

    int status = 0;
    while (::waitpid(pid, &status, 0) < 0)
    {
        if (errno == EINTR)
            continue;
        return false;
    }
    return true;
}

std::string temporaryRoot()
{
    std::string pattern =
        "/tmp/vdr-suite-hbbtv-media-runtime-test-XXXXXX";
    std::vector<char> writable(
        pattern.begin(),
        pattern.end());
    writable.push_back('\0');
    char* created =
        ::mkdtemp(writable.data());
    assert(created != nullptr);
    return std::string(created);
}

ClientMediaCapabilities browserCapabilities()
{
    ClientMediaCapabilities client;
    client.protocols = {
        MediaDeliveryProtocol::Progressive};
    client.containers = {
        MediaContainer::Fmp4};
    client.videoCodecs = {
        MediaCodec::H264};
    client.audioCodecs = {
        MediaCodec::Aac};
    client.maxVideoWidth = 1920;
    client.maxVideoHeight = 1080;
    client.maxAudioChannels = 2;
    return client;
}

MediaSessionIssuanceResult issue(
    MediaSessionIssuanceService& service,
    std::uint64_t revision)
{
    MediaSessionIssuanceRequest request;
    request.actorId =
        "actor_hbbtv_media";
    request.backendId = "default";
    request.resourceKind =
        "hbbtv-media";
    request.resourceId =
        "bas_hbbtv_test:" +
        std::to_string(revision);
    request.presentationProfileId =
        "hbbtv-media-negotiating";
    request.providerId =
        "vdr-plugin-web-hbbtv";
    request.lifetimeSeconds = 3600;
    return service.issue(request);
}

bool pair(
    const std::vector<std::string>& values,
    const std::string& option,
    const std::string& value)
{
    for (std::size_t index = 0;
         index + 1 < values.size();
         ++index)
    {
        if (values[index] == option &&
            values[index + 1] == value)
            return true;
    }
    return false;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MediaSessionRepository sessions(database);
    assert(sessions.ensureSchema());

    Entropy entropy;
    MediaSessionIssuanceService issuance(
        sessions,
        [&entropy](
            unsigned char* output,
            std::size_t size) {
            return entropy.fill(
                output,
                size);
        },
        [] {
            return std::chrono::
                system_clock::now();
        });

    std::vector<std::string> workerArgv;
    const std::string workspaceRoot =
        temporaryRoot();

    MediaTranscodePolicyConfig policyConfig;
    policyConfig.videoEncoderMode =
        MediaVideoEncoderMode::Software;

    HbbtvMediaSessionRuntime runtime(
        sessions,
        workspaceRoot,
        [&workerArgv](
            const std::vector<std::string>& argv,
            const std::string&,
            const std::string&) {
            workerArgv = argv;
            return spawnIdleWorker();
        },
        [](pid_t pid,
           std::chrono::milliseconds) {
            return terminateWorker(pid);
        },
        MediaTranscodePolicy(policyConfig));

    HbbtvMediaSource source;
    source.available = true;
    source.state =
        HbbtvMediaSourceState::Streaming;
    source.mediaRevision = 3;
    source.fullscreen = true;
    source.unixSocketPath =
        "/tmp/vdr-suite-hbbtv-media-test.sock";

    auto first = issue(issuance, 3);
    assert(first.issued);
    const auto firstProvision =
        runtime.provisionStream(
            first.session.sessionId,
            first.session.workspaceId,
            first.session.grantId,
            "bas_hbbtv_test",
            "default",
            source,
            browserCapabilities());
    assert(firstProvision.ready);
    assert(
        firstProvision.presentation.profileId ==
        "live-progressive-fmp4");
    assert(runtime.activeCount() == 1);
    assert(pair(
        workerArgv,
        "-i",
        "unix:///tmp/vdr-suite-hbbtv-media-test.sock"));
    assert(pair(
        workerArgv,
        "-c:v",
        "libx264"));
    assert(pair(
        workerArgv,
        "-c:a",
        "aac"));
    assert(std::count(
        workerArgv.begin(),
        workerArgv.end(),
        "unix:///tmp/vdr-suite-hbbtv-media-test.sock") ==
        1);

    const auto active =
        runtime.activeForApplicationSession(
            "bas_hbbtv_test");
    assert(active.has_value());
    assert(active->mediaRevision == 3);
    assert(
        active->mediaSessionId ==
        first.session.sessionId);

    assert(
        sessions.
            updateProvisioningPresentationProfile(
                first.session.sessionId,
                firstProvision.presentation.profileId));
    assert(
        sessions.activateBundle(
            first.session.sessionId));

    assert(
        runtime.stopForApplicationSession(
            "bas_hbbtv_test",
            "hbbtv_media_stopped"));
    assert(runtime.activeCount() == 0);

    const auto firstStored =
        sessions.findSession(
            first.session.sessionId);
    assert(firstStored.has_value());
    assert(firstStored->state == "ended");
    assert(
        firstStored->terminalReason ==
        "hbbtv_media_stopped");

    source.state =
        HbbtvMediaSourceState::Paused;
    source.mediaRevision = 4;

    auto second = issue(issuance, 4);
    assert(second.issued);
    const auto secondProvision =
        runtime.provisionStream(
            second.session.sessionId,
            second.session.workspaceId,
            second.session.grantId,
            "bas_hbbtv_test",
            "default",
            source,
            browserCapabilities());
    assert(secondProvision.ready);
    assert(
        sessions.
            updateProvisioningPresentationProfile(
                second.session.sessionId,
                secondProvision.presentation.profileId));
    assert(
        sessions.activateBundle(
            second.session.sessionId));

    assert(database.execute(
        std::string(
            "UPDATE media_access_grants "
            "SET active=0, "
            "revoked_at=CURRENT_TIMESTAMP "
            "WHERE grant_id='") +
        second.session.grantId + "';"));
    assert(runtime.reapInactive(60) == 1);
    assert(runtime.activeCount() == 0);

    const auto secondStored =
        sessions.findSession(
            second.session.sessionId);
    assert(secondStored.has_value());
    assert(secondStored->state == "ended");
    assert(
        secondStored->terminalReason ==
        "media_access_revoked");

    std::filesystem::remove_all(
        workspaceRoot);
    return 0;
}
