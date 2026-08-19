#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentLiveProviderRuntime.h"
#include "Database.h"
#include "LiveMediaSessionRuntime.h"
#include "MediaSessionIssuanceService.h"
#include "MediaSessionRepository.h"
#include "SuiteBridgeLiveSourceTransport.h"

#include <cassert>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace
{
constexpr const char* BackendId = "default";
constexpr const char* ChannelId = "S19.2E-1-1019-10301";

class FakeLiveTransport final : public vdrsuite::agent::ISuiteBridgeLiveSourceTransport
{
public:
    std::string epoch = "pie_1";
    int openCount = 0;
    int closeCount = 0;
    int staleCloseCount = 0;

    vdrsuite::agent::SuiteBridgeCommandReply discoverLiveSource() override
    {
        return success(
            250,
            std::string("{\"providerId\":\"suitebridge:local\",") +
            "\"providerKind\":\"suitebridge\"," +
            "\"pluginInstanceEpoch\":\"" + epoch + "\"," +
            "\"providerGeneration\":1," +
            "\"capabilityRevision\":1," +
            "\"capability\":\"vdr.live.stream\"," +
            "\"available\":true}");
    }

    vdrsuite::agent::SuiteBridgeCommandReply openLiveSource(
        const vdrsuite::agent::SuiteBridgeLiveSourceOpenRequest& request) override
    {
        if (request.pluginInstanceEpoch != epoch || request.channelId != ChannelId)
            return success(555, "live_source_plugin_instance_epoch_stale");
        ++openCount;
        return success(
            250,
            std::string("vdr-suite-live/1 state=active receiverAttached=true channelId=") +
            request.channelId +
            " socket=/tmp/vdr-suite-live-runtime.sock pluginInstanceEpoch=" + epoch);
    }

    vdrsuite::agent::SuiteBridgeCommandReply closeLiveSource(
        const vdrsuite::agent::SuiteBridgeLiveSourceLeaseRequest& request) override
    {
        ++closeCount;
        if (request.pluginInstanceEpoch != epoch) {
            ++staleCloseCount;
            return success(555, "live_source_plugin_instance_epoch_stale");
        }
        return success(250, "vdr-suite-live/1 state=terminal reason=closed");
    }

    vdrsuite::agent::SuiteBridgeCommandReply statusLiveSource(
        const vdrsuite::agent::SuiteBridgeLiveSourceLeaseRequest& request) override
    {
        if (request.pluginInstanceEpoch != epoch)
            return success(555, "live_source_plugin_instance_epoch_stale");
        return success(
            250,
            std::string("vdr-suite-live/1 state=active reason=none receiverAttached=true channelId=") +
            ChannelId);
    }

private:
    static vdrsuite::agent::SuiteBridgeCommandReply success(
        int replyCode,
        const std::string& payload)
    {
        vdrsuite::agent::SuiteBridgeCommandReply reply;
        reply.transportStatus = vdrsuite::agent::SuiteBridgeTransportStatus::Success;
        reply.replyCode = replyCode;
        reply.payload = payload;
        return reply;
    }
};

class Entropy
{
public:
    bool fill(unsigned char* output, std::size_t size)
    {
        if (output == nullptr) return false;
        for (std::size_t index = 0; index < size; ++index)
            output[index] = static_cast<unsigned char>((counter_++ % 251U) + 1U);
        return true;
    }

private:
    unsigned int counter_ = 1;
};

ClientMediaCapabilities browserCapabilities()
{
    ClientMediaCapabilities client;
    client.protocols = {MediaDeliveryProtocol::Hls};
    client.containers = {MediaContainer::Fmp4};
    client.videoCodecs = {MediaCodec::H264};
    client.audioCodecs = {MediaCodec::Aac};
    client.maxVideoWidth = 1920;
    client.maxVideoHeight = 1080;
    client.maxAudioChannels = 2;
    return client;
}

MediaProcessCaptureResult probeResult()
{
    MediaProcessCaptureResult result;
    result.started = true;
    result.completed = true;
    result.success = true;
    result.exitCode = 0;
    result.output =
        "codec_type=video|codec_name=h264|width=1920|height=1080|r_frame_rate=25/1|field_order=progressive\n"
        "codec_type=audio|codec_name=aac|channels=2|tag:language=deu\n";
    return result;
}

pid_t spawnIdleWorker()
{
    const pid_t pid = ::fork();
    if (pid == 0) {
        for (;;) ::pause();
    }
    return pid;
}

bool terminateWorker(pid_t pid)
{
    if (pid <= 0) return false;
    if (::kill(pid, SIGTERM) != 0) return false;
    int status = 0;
    while (::waitpid(pid, &status, 0) < 0) {
        if (errno == EINTR) continue;
        return false;
    }
    return true;
}

std::string temporaryRoot()
{
    std::string pattern = "/tmp/vdr-suite-live-runtime-test-XXXXXX";
    std::vector<char> writable(pattern.begin(), pattern.end());
    writable.push_back('\0');
    char* created = ::mkdtemp(writable.data());
    assert(created != nullptr);
    return std::string(created);
}

MediaSessionIssuanceResult issueLive(
    MediaSessionIssuanceService& service)
{
    MediaSessionIssuanceRequest request;
    request.actorId = "actor_live_runtime";
    request.backendId = BackendId;
    request.resourceKind = "live-channel";
    request.resourceId = ChannelId;
    request.presentationProfileId = "hls-fmp4";
    request.providerId = "suitebridge:local";
    request.lifetimeSeconds = 3600;
    return service.issue(request);
}

void seedProviderAuthority(
    Database& database,
    BackendAgentCommandRepository& commands)
{
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,backend_generation,"
        "protocol_version,software_version,heartbeat_sequence,capability_revision,"
        "last_connected_at,last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_live','default','actor_agent_live','dev_live','cred_live',1,"
        "'inst_live',9,'vdr-suite-agent/1','test',2,1,100,100,4102444800,1,1);"));
    assert(database.execute(
        "INSERT INTO backend_agent_observation_cursors("
        "backend_id,observation_domain,agent_id,agent_instance_id,backend_generation,"
        "snapshot_generation,producer_sequence,resource_revision,payload_identity,"
        "captured_at,accepted_at) VALUES("
        "'default','channels','agt_live','inst_live',9,3,7,'channels-r1','payload-r1',100,100);"));
    assert(database.execute(
        "INSERT INTO backend_agent_channel_facts("
        "backend_id,channel_id,channel_number,name,provider,group_name,radio,encrypted,enabled,"
        "agent_id,agent_instance_id,backend_generation,snapshot_generation,producer_sequence,"
        "captured_at,resource_revision) VALUES("
        "'default','S19.2E-1-1019-10301',1,'Das Erste HD','ARD','Öffentlich',0,0,1,"
        "'agt_live','inst_live',9,3,7,100,'channels-r1');"));

    vdrsuite::agent::BackendAgentLocalProviderOwnership ownership;
    std::string reason;
    assert(commands.setLocalProviderOwnership(
        BackendId,
        vdrsuite::agent::BackendAgentLiveProviderAuthority::AuthorityDomain,
        "suitebridge:local",
        "suitebridge",
        {vdrsuite::agent::BackendAgentLiveProviderAuthority::RequiredCapability},
        100,
        ownership,
        reason));
    assert(reason == "local_provider_ownership_set");
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    MediaSessionRepository sessions(database);
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(sessions.ensureSchema());
    seedProviderAuthority(database, commands);

    FakeLiveTransport transport;
    vdrsuite::agent::BackendAgentLiveProviderRuntime providerRuntime(
        agents, commands, transport);
    auto preparation = providerRuntime.prepare(BackendId, ChannelId);
    assert(preparation.valid);
    assert(preparation.reasonCode == "live_provider_pinned");

    Entropy entropy;
    MediaSessionIssuanceService issuance(
        sessions,
        [&entropy](unsigned char* output, std::size_t size) {
            return entropy.fill(output, size);
        },
        [] { return std::chrono::system_clock::now(); });

    const std::string workspaceRoot = temporaryRoot();
    LiveMediaSessionRuntime runtime(
        sessions,
        providerRuntime,
        workspaceRoot,
        [](const std::vector<std::string>&,
           const std::string&,
           std::chrono::milliseconds,
           std::size_t) { return probeResult(); },
        [](const std::vector<std::string>&,
           const std::string&,
           const std::string&) { return spawnIdleWorker(); },
        [](pid_t pid, std::chrono::milliseconds) { return terminateWorker(pid); },
        [](const std::string&, MediaContainer) { return true; },
        MediaTranscodePolicy());

    // Provider epoch replacement must reap the old receiver/worker/session
    // fail-closed. CLOSE on the old epoch is accepted as terminal evidence.
    auto first = issueLive(issuance);
    assert(first.issued);
    auto firstProvision = runtime.provisionHls(
        first.session.sessionId,
        first.session.workspaceId,
        first.session.leaseId,
        first.session.grantId,
        preparation,
        browserCapabilities());
    assert(firstProvision.ready);
    assert(runtime.activeCount() == 1);
    assert(transport.openCount == 1);
    transport.epoch = "pie_2";
    assert(runtime.reapInactive(60) == 1);
    assert(runtime.activeCount() == 0);
    assert(transport.closeCount == 1);
    assert(transport.staleCloseCount == 1);
    const auto firstStored = sessions.findSession(first.session.sessionId);
    assert(firstStored.has_value());
    assert(firstStored->state == "ended");
    assert(firstStored->terminalReason == "local_provider_instance_epoch_changed");
    assert(!std::filesystem::exists(
        std::filesystem::path(workspaceRoot) / first.session.workspaceId));

    // A revoked MediaAccessGrant is an independent runtime cleanup trigger.
    preparation = providerRuntime.prepare(BackendId, ChannelId);
    assert(preparation.valid);
    auto second = issueLive(issuance);
    assert(second.issued);
    auto secondProvision = runtime.provisionHls(
        second.session.sessionId,
        second.session.workspaceId,
        second.session.leaseId,
        second.session.grantId,
        preparation,
        browserCapabilities());
    assert(secondProvision.ready);
    assert(runtime.activeCount() == 1);
    assert(database.execute(
        std::string("UPDATE media_access_grants SET active=0, revoked_at=CURRENT_TIMESTAMP ") +
        "WHERE grant_id='" + second.session.grantId + "';"));
    assert(runtime.reapInactive(60) == 1);
    assert(runtime.activeCount() == 0);
    const auto secondStored = sessions.findSession(second.session.sessionId);
    assert(secondStored.has_value());
    assert(secondStored->state == "ended");
    assert(secondStored->terminalReason == "media_access_revoked");
    assert(!std::filesystem::exists(
        std::filesystem::path(workspaceRoot) / second.session.workspaceId));

    // Loss of the durable grant row is an invariant failure and must not leave
    // a native receiver or FFmpeg worker running without authorization state.
    auto third = issueLive(issuance);
    assert(third.issued);
    auto thirdProvision = runtime.provisionHls(
        third.session.sessionId,
        third.session.workspaceId,
        third.session.leaseId,
        third.session.grantId,
        preparation,
        browserCapabilities());
    assert(thirdProvision.ready);
    assert(runtime.activeCount() == 1);
    assert(database.execute(
        std::string("DELETE FROM media_access_grants WHERE grant_id='") +
        third.session.grantId + "';"));
    assert(runtime.reapInactive(60) == 1);
    assert(runtime.activeCount() == 0);
    const auto thirdStored = sessions.findSession(third.session.sessionId);
    assert(thirdStored.has_value());
    assert(thirdStored->state == "ended");
    assert(thirdStored->terminalReason == "media_access_grant_missing");
    assert(!std::filesystem::exists(
        std::filesystem::path(workspaceRoot) / third.session.workspaceId));

    // Daemon shutdown owns all remaining Live workers, provider leases and
    // workspaces instead of leaving them available after process teardown.
    auto fourth = issueLive(issuance);
    assert(fourth.issued);
    auto fourthProvision = runtime.provisionHls(
        fourth.session.sessionId,
        fourth.session.workspaceId,
        fourth.session.leaseId,
        fourth.session.grantId,
        preparation,
        browserCapabilities());
    assert(fourthProvision.ready);
    assert(runtime.activeCount() == 1);
    runtime.stopAll();
    assert(runtime.activeCount() == 0);
    const auto fourthStored = sessions.findSession(fourth.session.sessionId);
    assert(fourthStored.has_value());
    assert(fourthStored->state == "ended");
    assert(fourthStored->terminalReason == "daemon_shutdown");
    assert(!std::filesystem::exists(
        std::filesystem::path(workspaceRoot) / fourth.session.workspaceId));

    std::filesystem::remove_all(workspaceRoot);
    return 0;
}
