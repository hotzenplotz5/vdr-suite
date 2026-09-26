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

    std::puts(
        "SuiteBridge local control transport Live/native/OSD tests passed");
    return 0;
}
