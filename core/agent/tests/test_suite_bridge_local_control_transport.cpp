#include "SuiteBridgeLocalControlTransport.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace vdrsuite::agent;
using namespace vdrsuite::agent::control;

namespace
{

class SlowCompatibilityTransport final :
    public ISuiteBridgeLiveSourceTransport
{
public:
    std::timed_mutex lane;
    std::atomic<int> statusCalls{0};

    SuiteBridgeCommandReply discoverLiveSource() override
    {
        return {SuiteBridgeTransportStatus::Success, 250, "{}", {}};
    }

    SuiteBridgeCommandReply openLiveSource(
        const SuiteBridgeLiveSourceOpenRequest&) override
    {
        return {SuiteBridgeTransportStatus::Success, 250, "{}", {}};
    }

    SuiteBridgeCommandReply closeLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override
    {
        return {SuiteBridgeTransportStatus::Success, 250, "{}", {}};
    }

    SuiteBridgeCommandReply statusLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override
    {
        ++statusCalls;
        std::lock_guard<std::timed_mutex> guard(lane);
        return {SuiteBridgeTransportStatus::Success, 250, "{}", {}};
    }
};

class NativeProbeCompatibility final :
    public IBackendAgentNativeProbeTransport
{
public:
    std::atomic<int> executeCalls{0};

    SuiteBridgeCommandReply discoverNativeProbe() override
    {
        return {SuiteBridgeTransportStatus::Success, 900, "{}", {}};
    }

    SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest&) override
    {
        ++executeCalls;
        return {SuiteBridgeTransportStatus::Success, 900, "{}", {}};
    }

    SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest&) override
    {
        return {SuiteBridgeTransportStatus::Success, 900, "{}", {}};
    }
};

class NativeProbeTimeoutTransport final :
    public IBackendAgentNativeProbeTransport
{
public:
    SuiteBridgeCommandReply discoverNativeProbe() override
    {
        return {SuiteBridgeTransportStatus::Timeout, 0, {}, "timeout"};
    }

    SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest&) override
    {
        return {SuiteBridgeTransportStatus::Timeout, 0, {}, "timeout"};
    }

    SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest&) override
    {
        return {SuiteBridgeTransportStatus::Timeout, 0, {}, "timeout"};
    }
};

class LocalCompatibility final : public ISuiteBridgeLocalTransport
{
public:
    std::atomic<int> calls{0};

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand) override
    {
        ++calls;
        return {
            SuiteBridgeTransportStatus::Success,
            900,
            "{\"osd_schema\":1,\"active\":false}",
            {}};
    }
};

class OsdInputCompatibility final :
    public ISuiteBridgeLegacyOsdInputTransport
{
public:
    std::atomic<int> executeCalls{0};

    bool legacyOsdInputAvailable() override
    {
        return true;
    }

    SuiteBridgeCommandReply executeLegacyOsdInput(
        const LegacyOsdInputCommand&,
        const std::string&) override
    {
        ++executeCalls;
        return {SuiteBridgeTransportStatus::Success, 900, "{}", {}};
    }
};

class OsdInputTimeoutTransport final :
    public ISuiteBridgeLegacyOsdInputTransport
{
public:
    bool legacyOsdInputAvailable() override
    {
        return true;
    }

    SuiteBridgeCommandReply executeLegacyOsdInput(
        const LegacyOsdInputCommand&,
        const std::string&) override
    {
        return {SuiteBridgeTransportStatus::Timeout, 0, {}, "timeout"};
    }
};


class HbbtvCompatibility final : public ISuiteBridgeHbbtvTransport
{
public:
    std::atomic<int> runtimeCalls{0};

    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string&) override
    {
        return {
            true,
            SuiteBridgeHbbtvTransportStatus::Success,
            250,
            "{}"};
    }

    SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest&) override
    {
        ++runtimeCalls;
        return {
            true,
            SuiteBridgeHbbtvTransportStatus::Success,
            250,
            "{}"};
    }
};

class HbbtvTimeoutTransport final : public ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string&) override
    {
        return {
            false,
            SuiteBridgeHbbtvTransportStatus::Timeout,
            0,
            {}};
    }

    SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest&) override
    {
        return {
            false,
            SuiteBridgeHbbtvTransportStatus::Timeout,
            0,
            {}};
    }
};


class TeletextCompatibility final : public ISuiteBridgeTeletextTransport
{
public:
    std::atomic<int> pageCalls{0};

    SuiteBridgeTeletextCommandReply discoverTeletext() override
    {
        return {
            true,
            SuiteBridgeTeletextTransportStatus::Success,
            250,
            "{}"};
    }

    SuiteBridgeTeletextCommandReply requestTeletextPage(
        const SuiteBridgeTeletextPageRequest&) override
    {
        ++pageCalls;
        return {
            true,
            SuiteBridgeTeletextTransportStatus::Success,
            250,
            "{}"};
    }
};

class TeletextTimeoutTransport final : public ISuiteBridgeTeletextTransport
{
public:
    SuiteBridgeTeletextCommandReply discoverTeletext() override
    {
        return {
            false,
            SuiteBridgeTeletextTransportStatus::Timeout,
            0,
            {}};
    }

    SuiteBridgeTeletextCommandReply requestTeletextPage(
        const SuiteBridgeTeletextPageRequest&) override
    {
        return {
            false,
            SuiteBridgeTeletextTransportStatus::Timeout,
            0,
            {}};
    }
};


class EpgTypeSnapshotCompatibility final :
    public ISuiteBridgeEpgTypeSnapshotTransport
{
public:
    std::atomic<int> calls{0};

    SuiteBridgeEpgTypeSnapshotTransportPage requestEpgTypeSnapshot(
        std::int64_t,
        std::int64_t,
        std::uint64_t,
        std::size_t) override
    {
        ++calls;
        SuiteBridgeEpgTypeSnapshotTransportPage page;
        page.transportSucceeded = true;
        page.transportStatus = SuiteBridgeReadTransportStatus::Success;
        page.payloadValid = true;
        page.replyCode = 250;
        page.nextOffset = 1;
        page.scanned = 1;
        page.done = true;
        return page;
    }
};

class EpgTypeSnapshotTimeoutTransport final :
    public ISuiteBridgeEpgTypeSnapshotTransport
{
public:
    SuiteBridgeEpgTypeSnapshotTransportPage requestEpgTypeSnapshot(
        std::int64_t,
        std::int64_t,
        std::uint64_t,
        std::size_t) override
    {
        SuiteBridgeEpgTypeSnapshotTransportPage page;
        page.transportStatus = SuiteBridgeReadTransportStatus::Timeout;
        return page;
    }
};


class RecordingMetadataCompatibility final :
    public ISuiteBridgeRecordingMetadataTransport
{
public:
    std::atomic<int> calls{0};

    SuiteBridgeRecordingMetadataCommandReply requestRecordingMetadata(
        const std::string&) override
    {
        ++calls;
        SuiteBridgeRecordingMetadataCommandReply reply;
        reply.transportSucceeded = true;
        reply.replyCode = 250;
        reply.payload = "{}";
        reply.transportStatus = SuiteBridgeReadTransportStatus::Success;
        return reply;
    }
};

class RecordingMetadataTimeoutTransport final :
    public ISuiteBridgeRecordingMetadataTransport
{
public:
    SuiteBridgeRecordingMetadataCommandReply requestRecordingMetadata(
        const std::string&) override
    {
        SuiteBridgeRecordingMetadataCommandReply reply;
        reply.transportStatus = SuiteBridgeReadTransportStatus::Timeout;
        return reply;
    }
};

void serveOperation(
    const std::string& path,
    Operation expectedOperation,
    int replyCode,
    std::string payload,
    std::size_t expectedNewlines = 0)
{
    const int listener =
        socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    assert(listener >= 0);

    sockaddr_un address{};
    address.sun_family = AF_UNIX;
    std::copy(path.begin(), path.end(), address.sun_path);
    address.sun_path[path.size()] = '\0';
    unlink(path.c_str());

    assert(bind(
        listener,
        reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0);
    assert(listen(listener, 4) == 0);

    const int client = accept(listener, nullptr, nullptr);
    assert(client >= 0);

    std::vector<std::uint8_t> bytes(MaximumRequestFrameBytes + 1);
    const ssize_t received =
        recv(client, bytes.data(), bytes.size(), MSG_TRUNC);
    assert(received > 0);
    bytes.resize(static_cast<std::size_t>(received));

    Request request;
    std::string reason;
    assert(decodeRequest(bytes, request, reason));
    assert(request.operation == expectedOperation);
    assert(std::count(
        request.payload.begin(),
        request.payload.end(),
        '\n') == static_cast<std::ptrdiff_t>(expectedNewlines));

    Response response;
    response.result =
        replyCode == 555
        ? Result::StaleRejected
        : replyCode == 900
        ? Result::Success
        : Result::NativeRejected;
    response.requestId = request.requestId;
    response.replyCode = replyCode;
    response.payload = std::move(payload);

    assert(encodeResponse(response, bytes));
    assert(send(client, bytes.data(), bytes.size(), 0) ==
        static_cast<ssize_t>(bytes.size()));

    close(client);
    close(listener);
    unlink(path.c_str());
}

void waitForSocket(const std::string& path)
{
    while (access(path.c_str(), F_OK) != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

LegacyOsdInputCommand validOsdInput()
{
    LegacyOsdInputCommand command;
    command.inputCommandId = "osd-command-1";
    command.legacyOsdSessionId = "session-1";
    command.sessionRevision = 1;
    command.viewerBindingId = "viewer-1";
    command.controllerLeaseId = "lease-1";
    command.controllerLeaseEpoch = 1;
    command.leaseRevision = 1;
    command.actorId = "actor-1";
    command.clientInstanceId = "client-1";
    command.backendId = "backend-1";
    command.backendGeneration = 1;
    command.osdSurfaceId = "primary-native-osd";
    command.osdEpoch = "0123456789abcdef0123456789abcdef";
    command.action = LegacyOsdInputAction::Down;
    command.deadline =
        static_cast<std::int64_t>(std::time(nullptr) + 2);
    command.correlationId = "correlation-1";
    return command;
}

} // namespace

int main()
{
    const std::string livePath =
        "/tmp/vdr-suite-local-control-client-" +
        std::to_string(getpid()) + ".sock";

    std::thread liveServer([&] {
        serveOperation(
            livePath,
            Operation::LiveStatus,
            250,
            "{\"state\":\"streaming\"}",
            1);
    });
    waitForSocket(livePath);

    SlowCompatibilityTransport compatibility;
    SuiteBridgeLocalControlTransportConfig liveConfig;
    liveConfig.socketPath = livePath;
    liveConfig.connectTimeout = std::chrono::milliseconds(100);
    liveConfig.ioTimeout = std::chrono::milliseconds(200);
    liveConfig.operationTimeout = std::chrono::milliseconds(300);
    SuiteBridgeLocalControlTransport liveDedicated(liveConfig);
    SuiteBridgePrioritizedLiveTransport liveRouted(
        liveDedicated,
        compatibility);

    compatibility.lane.lock();
    SuiteBridgeLiveSourceLeaseRequest liveRequest{"lease-1", "epoch-1"};
    const auto started = std::chrono::steady_clock::now();
    const auto liveReply = liveRouted.statusLiveSource(liveRequest);
    const auto elapsed =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - started);
    compatibility.lane.unlock();

    assert(liveReply.transportSucceeded());
    assert(liveReply.replyCode == 250);
    assert(elapsed < std::chrono::milliseconds(150));
    assert(compatibility.statusCalls.load() == 0);
    liveServer.join();

    SuiteBridgeLocalControlTransportConfig missingConfig;
    missingConfig.socketPath = livePath + ".missing";
    SuiteBridgeLocalControlTransport missing(missingConfig);
    SuiteBridgePrioritizedLiveTransport liveFallback(
        missing,
        compatibility);
    assert(liveFallback.statusLiveSource(liveRequest).transportSucceeded());
    assert(compatibility.statusCalls.load() == 1);

    const std::string nativePath =
        "/tmp/vdr-suite-native-probe-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread nativeServer([&] {
        serveOperation(
            nativePath,
            Operation::NativeProbeExecute,
            900,
            "{\"nativeOperation\":\"vdr.native.probe\"}",
            11);
    });
    waitForSocket(nativePath);

    SuiteBridgeLocalControlTransportConfig nativeConfig;
    nativeConfig.socketPath = nativePath;
    SuiteBridgeLocalControlTransport nativeDedicated(nativeConfig);
    NativeProbeCompatibility nativeCompatibility;
    SuiteBridgePrioritizedNativeProbeTransport nativeRouted(
        nativeDedicated,
        nativeCompatibility);

    SuiteBridgeNativeProbeRequest probe;
    probe.commandId = "command-1";
    probe.requestFingerprint = "fingerprint-1";
    probe.operationId = "operation-1";
    probe.jobId = "job-1";
    probe.attemptId = "attempt-1";
    probe.claimEpoch = 1;
    probe.backendId = "backend-1";
    probe.agentId = "agent-1";
    probe.agentInstanceId = "instance-1";
    probe.backendGeneration = 1;
    probe.pluginInstanceEpoch = "epoch-1";
    probe.probeNonce = "nonce-1";

    const auto probeReply = nativeRouted.executeNativeProbe(probe);
    assert(probeReply.transportSucceeded());
    assert(probeReply.replyCode == 900);
    assert(nativeCompatibility.executeCalls.load() == 0);
    nativeServer.join();

    SuiteBridgeLocalControlTransportConfig nativeMissingConfig;
    nativeMissingConfig.socketPath = nativePath + ".missing";
    SuiteBridgeLocalControlTransport nativeMissing(nativeMissingConfig);
    SuiteBridgePrioritizedNativeProbeTransport nativeFallback(
        nativeMissing,
        nativeCompatibility);
    assert(nativeFallback.executeNativeProbe(probe).transportSucceeded());
    assert(nativeCompatibility.executeCalls.load() == 1);

    NativeProbeTimeoutTransport nativeTimeout;
    SuiteBridgePrioritizedNativeProbeTransport nativeNoReplay(
        nativeTimeout,
        nativeCompatibility);
    assert(nativeNoReplay.executeNativeProbe(probe).transportStatus ==
        SuiteBridgeTransportStatus::Timeout);
    assert(nativeCompatibility.executeCalls.load() == 1);

    const std::string osdSnapshotPath =
        "/tmp/vdr-suite-osd-snapshot-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread osdSnapshotServer([&] {
        serveOperation(
            osdSnapshotPath,
            Operation::OsdSnapshot,
            900,
            "{\"osd_schema\":1,\"active\":false}");
    });
    waitForSocket(osdSnapshotPath);

    SuiteBridgeLocalControlTransportConfig osdConfig;
    osdConfig.socketPath = osdSnapshotPath;
    SuiteBridgeLocalControlTransport osdDedicated(osdConfig);
    LocalCompatibility osdCompatibility;
    SuiteBridgePrioritizedLocalTransport osdRead(
        osdDedicated,
        osdCompatibility);

    const auto osdSnapshotReply =
        osdRead.execute(SuiteBridgeLocalCommand::OsdSnapshot);
    assert(osdSnapshotReply.transportSucceeded());
    assert(osdSnapshotReply.replyCode == 900);
    assert(osdCompatibility.calls.load() == 0);
    osdSnapshotServer.join();

    SuiteBridgeLocalControlTransportConfig osdMissingConfig;
    osdMissingConfig.socketPath = osdSnapshotPath + ".missing";
    SuiteBridgeLocalControlTransport osdMissing(osdMissingConfig);
    SuiteBridgePrioritizedLocalTransport osdReadFallback(
        osdMissing,
        osdCompatibility);
    assert(osdReadFallback.execute(
        SuiteBridgeLocalCommand::OsdSnapshot).transportSucceeded());
    assert(osdCompatibility.calls.load() == 1);

    const std::string osdInputPath =
        "/tmp/vdr-suite-osd-input-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread osdInputServer([&] {
        serveOperation(
            osdInputPath,
            Operation::OsdInput,
            900,
            "{\"nativeOperation\":\"legacy-osd.input\"}",
            9);
    });
    waitForSocket(osdInputPath);

    SuiteBridgeLocalControlTransportConfig osdInputConfig;
    osdInputConfig.socketPath = osdInputPath;
    SuiteBridgeLocalControlTransport osdInputDedicated(osdInputConfig);
    OsdInputCompatibility osdInputCompatibility;
    SuiteBridgePrioritizedLegacyOsdInputTransport osdInputRouted(
        osdCompatibility,
        osdInputDedicated,
        osdInputCompatibility);

    const LegacyOsdInputCommand osdCommand = validOsdInput();
    const auto osdInputReply =
        osdInputRouted.executeLegacyOsdInput(
            osdCommand,
            "fingerprint-1");
    assert(osdInputReply.transportSucceeded());
    assert(osdInputReply.replyCode == 900);
    assert(osdInputCompatibility.executeCalls.load() == 0);
    osdInputServer.join();

    SuiteBridgeLocalControlTransportConfig osdInputMissingConfig;
    osdInputMissingConfig.socketPath = osdInputPath + ".missing";
    SuiteBridgeLocalControlTransport osdInputMissing(
        osdInputMissingConfig);
    SuiteBridgePrioritizedLegacyOsdInputTransport osdInputFallback(
        osdCompatibility,
        osdInputMissing,
        osdInputCompatibility);
    assert(osdInputFallback.executeLegacyOsdInput(
        osdCommand,
        "fingerprint-1").transportSucceeded());
    assert(osdInputCompatibility.executeCalls.load() == 1);

    OsdInputTimeoutTransport osdInputTimeout;
    SuiteBridgePrioritizedLegacyOsdInputTransport osdInputNoReplay(
        osdCompatibility,
        osdInputTimeout,
        osdInputCompatibility);
    assert(osdInputNoReplay.executeLegacyOsdInput(
        osdCommand,
        "fingerprint-1").transportStatus ==
        SuiteBridgeTransportStatus::Timeout);
    assert(osdInputCompatibility.executeCalls.load() == 1);


    const std::string hbbtvPath =
        "/tmp/vdr-suite-hbbtv-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread hbbtvServer([&] {
        serveOperation(
            hbbtvPath,
            Operation::HbbtvRuntime,
            250,
            "{\"state\":\"active\"}");
    });
    waitForSocket(hbbtvPath);

    SuiteBridgeLocalControlTransportConfig hbbtvConfig;
    hbbtvConfig.socketPath = hbbtvPath;
    SuiteBridgeLocalControlTransport hbbtvDedicated(hbbtvConfig);
    HbbtvCompatibility hbbtvCompatibility;
    SuiteBridgePrioritizedHbbtvTransport hbbtvRouted(
        hbbtvDedicated,
        hbbtvCompatibility);

    SuiteBridgeHbbtvRuntimeRequest hbbtvRequest;
    hbbtvRequest.operation = SuiteBridgeHbbtvRuntimeOperation::Status;
    hbbtvRequest.sessionId = "session-a";
    hbbtvRequest.channelId = "C-1-1051-10301";
    hbbtvRequest.applicationId = 1;
    hbbtvRequest.descriptorRevision = 2;

    const auto hbbtvReply = hbbtvRouted.controlHbbtv(hbbtvRequest);
    assert(hbbtvReply.transportSucceeded);
    assert(hbbtvReply.transportStatus ==
        SuiteBridgeHbbtvTransportStatus::Success);
    assert(hbbtvReply.replyCode == 250);
    assert(hbbtvCompatibility.runtimeCalls.load() == 0);
    hbbtvServer.join();

    SuiteBridgeLocalControlTransportConfig hbbtvMissingConfig;
    hbbtvMissingConfig.socketPath = hbbtvPath + ".missing";
    SuiteBridgeLocalControlTransport hbbtvMissing(hbbtvMissingConfig);
    SuiteBridgePrioritizedHbbtvTransport hbbtvFallback(
        hbbtvMissing,
        hbbtvCompatibility);
    assert(hbbtvFallback.controlHbbtv(hbbtvRequest).transportSucceeded);
    assert(hbbtvCompatibility.runtimeCalls.load() == 1);

    HbbtvTimeoutTransport hbbtvTimeout;
    SuiteBridgePrioritizedHbbtvTransport hbbtvNoReplay(
        hbbtvTimeout,
        hbbtvCompatibility);
    const auto hbbtvTimeoutReply =
        hbbtvNoReplay.controlHbbtv(hbbtvRequest);
    assert(!hbbtvTimeoutReply.transportSucceeded);
    assert(hbbtvTimeoutReply.transportStatus ==
        SuiteBridgeHbbtvTransportStatus::Timeout);
    assert(hbbtvCompatibility.runtimeCalls.load() == 1);


    const std::string teletextPath =
        "/tmp/vdr-suite-teletext-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread teletextServer([&] {
        serveOperation(
            teletextPath,
            Operation::TeletextPage,
            250,
            "{\"result\":\"ok\"}");
    });
    waitForSocket(teletextPath);

    SuiteBridgeLocalControlTransportConfig teletextConfig;
    teletextConfig.socketPath = teletextPath;
    SuiteBridgeLocalControlTransport teletextDedicated(teletextConfig);
    TeletextCompatibility teletextCompatibility;
    SuiteBridgePrioritizedTeletextTransport teletextRouted(
        teletextDedicated,
        teletextCompatibility);

    SuiteBridgeTeletextPageRequest teletextRequest;
    teletextRequest.channelId = "C-1-1051-10301";
    teletextRequest.pageNumber = 100;
    teletextRequest.automaticSubpage = true;

    const auto teletextReply =
        teletextRouted.requestTeletextPage(teletextRequest);
    assert(teletextReply.transportSucceeded);
    assert(teletextReply.transportStatus ==
        SuiteBridgeTeletextTransportStatus::Success);
    assert(teletextCompatibility.pageCalls.load() == 0);
    teletextServer.join();

    SuiteBridgeLocalControlTransportConfig teletextMissingConfig;
    teletextMissingConfig.socketPath = teletextPath + ".missing";
    SuiteBridgeLocalControlTransport teletextMissing(
        teletextMissingConfig);
    SuiteBridgePrioritizedTeletextTransport teletextFallback(
        teletextMissing,
        teletextCompatibility);
    assert(teletextFallback.requestTeletextPage(
        teletextRequest).transportSucceeded);
    assert(teletextCompatibility.pageCalls.load() == 1);

    TeletextTimeoutTransport teletextTimeout;
    SuiteBridgePrioritizedTeletextTransport teletextNoReplay(
        teletextTimeout,
        teletextCompatibility);
    const auto teletextTimeoutReply =
        teletextNoReplay.requestTeletextPage(teletextRequest);
    assert(!teletextTimeoutReply.transportSucceeded);
    assert(teletextTimeoutReply.transportStatus ==
        SuiteBridgeTeletextTransportStatus::Timeout);
    assert(teletextCompatibility.pageCalls.load() == 1);


    const std::string epgTypeSnapshotPath =
        "/tmp/vdr-suite-epg-type-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread epgTypeSnapshotServer([&] {
        serveOperation(
            epgTypeSnapshotPath,
            Operation::EpgTypeSnapshot,
            250,
            "1|1|1|1|C-1-1051-10301,42,100,200,S");
    });
    waitForSocket(epgTypeSnapshotPath);

    SuiteBridgeLocalControlTransportConfig epgTypeSnapshotConfig;
    epgTypeSnapshotConfig.socketPath = epgTypeSnapshotPath;
    SuiteBridgeLocalControlTransport epgTypeSnapshotDedicated(
        epgTypeSnapshotConfig);
    EpgTypeSnapshotCompatibility epgTypeSnapshotCompatibility;
    SuiteBridgePrioritizedEpgTypeSnapshotTransport epgTypeSnapshotRouted(
        epgTypeSnapshotDedicated,
        epgTypeSnapshotCompatibility);

    const auto epgTypeSnapshotReply =
        epgTypeSnapshotRouted.requestEpgTypeSnapshot(50, 250, 0, 64);
    assert(epgTypeSnapshotReply.transportSucceeded);
    assert(epgTypeSnapshotReply.transportStatus ==
        SuiteBridgeReadTransportStatus::Success);
    assert(epgTypeSnapshotReply.payloadValid);
    assert(epgTypeSnapshotReply.replyCode == 250);
    assert(epgTypeSnapshotReply.nextOffset == 1);
    assert(epgTypeSnapshotReply.scanned == 1);
    assert(epgTypeSnapshotReply.done);
    assert(epgTypeSnapshotReply.items.size() == 1);
    assert(epgTypeSnapshotReply.items[0].mediaType ==
        EpgScraperMediaType::Series);
    assert(epgTypeSnapshotCompatibility.calls.load() == 0);
    epgTypeSnapshotServer.join();

    SuiteBridgeLocalControlTransportConfig epgTypeSnapshotMissingConfig;
    epgTypeSnapshotMissingConfig.socketPath =
        epgTypeSnapshotPath + ".missing";
    SuiteBridgeLocalControlTransport epgTypeSnapshotMissing(
        epgTypeSnapshotMissingConfig);
    SuiteBridgePrioritizedEpgTypeSnapshotTransport epgTypeSnapshotFallback(
        epgTypeSnapshotMissing,
        epgTypeSnapshotCompatibility);
    const auto epgTypeSnapshotFallbackReply =
        epgTypeSnapshotFallback.requestEpgTypeSnapshot(50, 250, 0, 64);
    assert(epgTypeSnapshotFallbackReply.transportSucceeded);
    assert(epgTypeSnapshotCompatibility.calls.load() == 1);

    EpgTypeSnapshotTimeoutTransport epgTypeSnapshotTimeout;
    SuiteBridgePrioritizedEpgTypeSnapshotTransport epgTypeSnapshotNoReplay(
        epgTypeSnapshotTimeout,
        epgTypeSnapshotCompatibility);
    const auto epgTypeSnapshotTimeoutReply =
        epgTypeSnapshotNoReplay.requestEpgTypeSnapshot(50, 250, 0, 64);
    assert(!epgTypeSnapshotTimeoutReply.transportSucceeded);
    assert(epgTypeSnapshotTimeoutReply.transportStatus ==
        SuiteBridgeReadTransportStatus::Timeout);
    assert(epgTypeSnapshotCompatibility.calls.load() == 1);


    const std::string recordingMetadataPath =
        "/tmp/vdr-suite-recording-metadata-control-" +
        std::to_string(getpid()) + ".sock";

    std::thread recordingMetadataServer([&] {
        serveOperation(
            recordingMetadataPath,
            Operation::RecordingMetadata,
            250,
            "{\"found\":true}");
    });
    waitForSocket(recordingMetadataPath);

    SuiteBridgeLocalControlTransportConfig recordingMetadataConfig;
    recordingMetadataConfig.socketPath = recordingMetadataPath;
    SuiteBridgeLocalControlTransport recordingMetadataDedicated(
        recordingMetadataConfig);
    RecordingMetadataCompatibility recordingMetadataCompatibility;
    SuiteBridgePrioritizedRecordingMetadataTransport
        recordingMetadataRouted(
            recordingMetadataDedicated,
            recordingMetadataCompatibility);

    const std::string recordingKey =
        "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
    const auto recordingMetadataReply =
        recordingMetadataRouted.requestRecordingMetadata(recordingKey);
    assert(recordingMetadataReply.transportSucceeded);
    assert(recordingMetadataReply.transportStatus ==
        SuiteBridgeReadTransportStatus::Success);
    assert(recordingMetadataCompatibility.calls.load() == 0);
    recordingMetadataServer.join();

    SuiteBridgeLocalControlTransportConfig recordingMetadataMissingConfig;
    recordingMetadataMissingConfig.socketPath =
        recordingMetadataPath + ".missing";
    SuiteBridgeLocalControlTransport recordingMetadataMissing(
        recordingMetadataMissingConfig);
    SuiteBridgePrioritizedRecordingMetadataTransport
        recordingMetadataFallback(
            recordingMetadataMissing,
            recordingMetadataCompatibility);
    assert(recordingMetadataFallback.requestRecordingMetadata(
        recordingKey).transportSucceeded);
    assert(recordingMetadataCompatibility.calls.load() == 1);

    RecordingMetadataTimeoutTransport recordingMetadataTimeout;
    SuiteBridgePrioritizedRecordingMetadataTransport
        recordingMetadataNoReplay(
            recordingMetadataTimeout,
            recordingMetadataCompatibility);
    const auto recordingMetadataTimeoutReply =
        recordingMetadataNoReplay.requestRecordingMetadata(recordingKey);
    assert(!recordingMetadataTimeoutReply.transportSucceeded);
    assert(recordingMetadataTimeoutReply.transportStatus ==
        SuiteBridgeReadTransportStatus::Timeout);
    assert(recordingMetadataCompatibility.calls.load() == 1);

    std::puts(
        "SuiteBridge local control transport Live/native/OSD/HbbTV/Teletext/provider/ETYPES tests passed");
    return 0;
}
