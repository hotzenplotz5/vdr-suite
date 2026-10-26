#include "BackendAgentClient.h"
#include "BackendAgentCommandClient.h"
#include "BackendAgentRecordingMarksModify.h"
#include "BackendAgentRecordingCut.h"
#include "BackendAgentMediaImportRuntime.h"
#include "BackendAgentMediaShrinkAuthorization.h"
#include "LocalMediaShrinkAgentExecutor.h"
#include "ProtectedMediaShrinkAgentExecutor.h"
#include "AddonInstallSnapshot.h"
#include "MediaShrinkAgentCommand.h"
#include "MediaToolsAgentExecutor.h"
#include "SuiteBridgeNativeTimerCreateTransport.h"
#include "SuiteBridgeNativeTimerDeleteTransport.h"
#include "SuiteBridgeNativeTimerModifyTransport.h"
#include "SuiteBridgeRecordingMarksModifyTransport.h"
#include "SuiteBridgeRecordingCutTransport.h"
#include "SuiteBridgeLocalControlTransport.h"
#include "SuiteBridgeSvdrpTransport.h"
#include "SuiteBridgeHandshakeService.h"
#include "SuiteBridgeOsdFrameSource.h"
#include "LegacyOsdInputDomain.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <chrono>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <memory>
#include <string>

namespace
{
std::atomic<bool> StopRequested(false);

void handleSignal(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) StopRequested.store(true);
}

bool loopbackHost(const std::string& host)
{
    return host == "127.0.0.1" || host == "::1" || host == "localhost";
}

bool portNumber(const std::string& value, std::uint16_t& port)
{
    if (value.empty()) return false;
    errno = 0;
    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' ||
        parsed == 0 || parsed > 65535) return false;
    port = static_cast<std::uint16_t>(parsed);
    return true;
}

void usage()
{
    std::cerr
        << "usage: vdr-suite-backend-agent [--config PATH] "
           "[--once | --rotate-credential] "
           "[--native-probe --suitebridge-host LOOPBACK "
           "--suitebridge-port PORT] [--enable-media-import] "
           "[--enable-media-shrink --media-shrink-state-dir ABSOLUTE]" << std::endl;
}
}

int main(int argc, char** argv)
{
    std::string configPath = "/etc/vdr-suite/backend-agent.conf";
    bool once = false;
    bool rotateCredential = false;
    bool nativeProbe = false;
    bool enableMediaImport = false;
    bool enableMediaShrink = false;
    std::string shrinkStateDirectory;
    std::string suiteBridgeHost;
    std::uint16_t suiteBridgePort = 0;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--config" && index + 1 < argc)
        {
            configPath = argv[++index];
        }
        else if (argument == "--once")
        {
            once = true;
        }
        else if (argument == "--rotate-credential")
        {
            rotateCredential = true;
        }
        else if (argument == "--native-probe")
        {
            nativeProbe = true;
        }
        else if (argument == "--enable-media-import")
        {
            enableMediaImport = true;
        }
        else if (argument == "--enable-media-shrink")
        {
            enableMediaShrink = true;
        }
        else if (argument == "--media-shrink-state-dir" && index + 1 < argc)
        {
            shrinkStateDirectory = argv[++index];
        }
        else if (argument == "--suitebridge-host" && index + 1 < argc)
        {
            suiteBridgeHost = argv[++index];
        }
        else if (argument == "--suitebridge-port" && index + 1 < argc)
        {
            if (!portNumber(argv[++index], suiteBridgePort))
            {
                usage();
                return 64;
            }
        }
        else
        {
            usage();
            return 64;
        }
    }
    if ((once && rotateCredential) ||
        ((enableMediaImport || enableMediaShrink) &&
         (nativeProbe || once || rotateCredential)) ||
        (enableMediaShrink != !shrinkStateDirectory.empty()))
    {
        usage();
        return 64;
    }
    if (nativeProbe)
    {
        if (!loopbackHost(suiteBridgeHost) || suiteBridgePort == 0)
        {
            std::cerr << "native probe requires an explicit loopback SuiteBridge host and port" << std::endl;
            return 78;
        }
    }
    else if (!suiteBridgeHost.empty() || suiteBridgePort != 0)
    {
        usage();
        return 64;
    }

    BackendAgentClientConfig config;
    std::string reason;
    if (!BackendAgentClientRuntime::loadConfig(configPath, config, reason))
    {
        std::cerr << "Backend Agent configuration rejected: " << reason << std::endl;
        return 78;
    }

    // The agent's explicit configuration has priority over the private
    // process environment shared by the daemon and the VDR plugin.
    vdrsuite::agent::SuiteBridgeLocalControlTransportConfig localControlConfig;
    if (!config.suiteBridgeControlSocketPath.empty())
    {
        localControlConfig.socketPath = config.suiteBridgeControlSocketPath;
    }
    if (!vdrsuite::agent::validSuiteBridgeControlSocketPath(
            localControlConfig.socketPath))
    {
        std::cerr << "Backend Agent control socket path rejected" << std::endl;
        return 78;
    }

    // Native recording mutations are activated by the binary only when the
    // configured SuiteBridge endpoint is present. Keeping them out of the
    // user-editable COMMAND_TYPES allowlist preserves the established config
    // parser contract while the runtime advertises each command only after
    // live SuiteBridge capability discovery below.
    const bool osdReadOnlyConfiguration = config.commandTypes.empty() &&
        std::find(config.observationDomains.begin(), config.observationDomains.end(),
                  "osd") != config.observationDomains.end();
    if (!config.suiteBridgeHost.empty() && !osdReadOnlyConfiguration)
    {
        if (std::find(
                config.commandTypes.begin(),
                config.commandTypes.end(),
                vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType) ==
            config.commandTypes.end())
        {
            config.commandTypes.push_back(
                vdrsuite::agent::kBackendAgentRecordingMarksModifyCommandType);
        }
        if (std::find(
                config.commandTypes.begin(),
                config.commandTypes.end(),
                vdrsuite::agent::kBackendAgentRecordingCutCommandType) ==
            config.commandTypes.end())
        {
            config.commandTypes.push_back(
                vdrsuite::agent::kBackendAgentRecordingCutCommandType);
        }
    }

    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerCreateTransport>
        nativeTimerCreateTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerDeleteTransport>
        nativeTimerDeleteTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeNativeTimerModifyTransport>
        nativeTimerModifyTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeRecordingMarksModifyTransport>
        recordingMarksModifyTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeRecordingCutTransport>
        recordingCutTransport;
    if (!config.suiteBridgeHost.empty())
    {
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig timerTransportConfig;
        timerTransportConfig.host = config.suiteBridgeHost;
        timerTransportConfig.port = config.suiteBridgePort;
        nativeTimerCreateTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerCreateTransport>(
                timerTransportConfig);
        nativeTimerDeleteTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerDeleteTransport>(
                timerTransportConfig);
        // Native UPDATE/TOGGLE executes on VDR's main-thread timer lock.
        // An otherwise healthy, locally busy VDR can exceed the generic
        // 1-second SVDRP reply deadline. Give only MODIFY a realistic
        // bounded budget; an ambiguous timeout is still outcome_unknown
        // and is NEVER dispatched a second time.
        auto modifyTransportConfig = timerTransportConfig;
        modifyTransportConfig.ioTimeout = std::chrono::milliseconds(4000);
        modifyTransportConfig.operationTimeout =
            std::chrono::milliseconds(12000);
        nativeTimerModifyTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeNativeTimerModifyTransport>(
                modifyTransportConfig);
        recordingMarksModifyTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeRecordingMarksModifyTransport>(
                timerTransportConfig);
        recordingCutTransport = std::make_unique<
            vdrsuite::agent::SuiteBridgeRecordingCutTransport>(
                timerTransportConfig);
        config.nativeTimerCreateTransport =
            nativeTimerCreateTransport.get();
        config.nativeTimerDeleteTransport =
            nativeTimerDeleteTransport.get();
        config.nativeTimerModifyTransport =
            nativeTimerModifyTransport.get();
        setBackendAgentRecordingCutTransport(recordingCutTransport.get());
        setBackendAgentRecordingMarksModifyTransport(
            recordingMarksModifyTransport.get());
    }

    std::unique_ptr<vdrsuite::agent::SuiteBridgeSvdrpTransport> osdTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeLocalControlTransport>
        osdLocalControlTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgePrioritizedLocalTransport>
        osdReadTransport;
    std::unique_ptr<
        vdrsuite::agent::SuiteBridgePrioritizedLegacyOsdInputTransport>
        osdInputTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeOsdFrameSource> osdSource;
    std::uint64_t osdGeneration = 0;
    if (std::find(config.observationDomains.begin(), config.observationDomains.end(),
                  "osd") != config.observationDomains.end() && !config.suiteBridgeHost.empty())
    {
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig osdConfig;
        osdConfig.host = config.suiteBridgeHost;
        osdConfig.port = config.suiteBridgePort;
        osdTransport =
            std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(
                osdConfig);
        osdLocalControlTransport =
            std::make_unique<
                vdrsuite::agent::SuiteBridgeLocalControlTransport>(
                    localControlConfig);
        osdReadTransport =
            std::make_unique<
                vdrsuite::agent::SuiteBridgePrioritizedLocalTransport>(
                    *osdLocalControlTransport,
                    *osdTransport);
        osdInputTransport =
            std::make_unique<
                vdrsuite::agent::SuiteBridgePrioritizedLegacyOsdInputTransport>(
                    *osdReadTransport,
                    *osdLocalControlTransport,
                    *osdTransport);
        config.legacyOsdInputTransport = osdInputTransport.get();
        if (std::find(
                config.commandTypes.begin(),
                config.commandTypes.end(),
                kLegacyOsdInputCommandType) == config.commandTypes.end())
        {
            config.commandTypes.push_back(kLegacyOsdInputCommandType);
        }
        // Interactive OSD input has a three-second dispatch deadline, so it
        // cannot wait for the normal heartbeat cadence. Reuse the existing
        // authenticated command poll path at a bounded low-latency cadence.
        config.commandPollIntervalMilliseconds = 250;
        config.osdObservationSource = [&](const std::string& backendId, std::uint64_t generation) {
            if (!osdSource || osdGeneration != generation)
            {
                osdGeneration = generation;
                osdSource = std::make_unique<vdrsuite::agent::SuiteBridgeOsdFrameSource>(
                    *osdReadTransport, backendId, generation);
            }
            vdrsuite::agent::SuiteBridgeHandshakeService handshake(*osdReadTransport);
            const auto discovery = handshake.discover();
            return osdSource->read(discovery.compatible()
                ? discovery.discovery : vdrsuite::agent::SuiteBridgeDiscovery{});
        };
    }

    std::unique_ptr<vdrsuite::agent::SuiteBridgeSvdrpTransport>
        nativeCompatibilityTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgeLocalControlTransport>
        nativeLocalControlTransport;
    std::unique_ptr<vdrsuite::agent::SuiteBridgePrioritizedNativeProbeTransport>
        nativeTransport;
    if (nativeProbe)
    {
        if (!config.commandTypes.empty())
        {
            std::cerr << "native probe CLI activation requires COMMAND_TYPES to remain empty" << std::endl;
            return 78;
        }
        config.commandTypes = {"vdr.native.probe"};
        vdrsuite::agent::SuiteBridgeSvdrpTransportConfig transportConfig;
        transportConfig.host = suiteBridgeHost;
        transportConfig.port = suiteBridgePort;
        nativeCompatibilityTransport =
            std::make_unique<vdrsuite::agent::SuiteBridgeSvdrpTransport>(
                transportConfig);
        nativeLocalControlTransport =
            std::make_unique<vdrsuite::agent::SuiteBridgeLocalControlTransport>(
                localControlConfig);
        nativeTransport =
            std::make_unique<
                vdrsuite::agent::SuiteBridgePrioritizedNativeProbeTransport>(
                    *nativeLocalControlTransport,
                    *nativeCompatibilityTransport);
        setBackendAgentNativeProbeTransport(nativeTransport.get());
    }

    CurlBackendAgentControlPlaneTransport transport(config);

    // Production defaults to no Media Import executor. Neither installed
    // metadata nor COMMAND_TYPES may opt in. A locally explicit CLI switch
    // only constructs the gated composition; it cannot bypass the server's
    // per-backend administrator intent, operation and effective grant fence.
    BackendAgentClientRuntime* runtimeAccess = nullptr;
    std::unique_ptr<vdrsuite::agent::BackendAgentMediaImportRuntime>
        mediaImportRuntime;
    if (enableMediaImport)
    {
        if (std::find(config.commandTypes.begin(), config.commandTypes.end(),
                      "probe.noop") != config.commandTypes.end())
        {
            std::cerr << "media import cannot be combined with probe.noop"
                      << std::endl;
            return 78;
        }
        mediaImportRuntime = std::make_unique<
            vdrsuite::agent::BackendAgentMediaImportRuntime>(
                "/", config.backendId, config.commandStatePath, transport,
                [&]() {
                    BackendAgentCommandClientContext context;
                    if (runtimeAccess)
                    {
                        const auto& state = runtimeAccess->state();
                        context.agentId = state.agentId;
                        context.credentialSecret = state.credentialSecret;
                        context.backendId = state.backendId;
                        context.agentInstanceId = runtimeAccess->agentInstanceId();
                        context.backendGeneration = state.backendGeneration;
                    }
                    return context;
                });
        config.mediaImportExecutor = mediaImportRuntime.get();
        config.commandTypes.push_back(
            vdrsuite::mediatools::kMediaImportCommandType);
    }
    // HEVC is NEVER enabled by COMMAND_TYPES or manifest presence alone.
    // Only the explicit local CLI opt-in constructs the authenticated,
    // receipt-revalidating encoder, with an existing private owner-only
    // journal. No directory is created or permissions repaired implicitly.
    std::unique_ptr<vdrsuite::mediatools::LocalMediaShrinkReviewStore>
        shrinkReviews;
    std::unique_ptr<vdrsuite::agent::ReceiptRevalidatingMediaShrinkAuthorization>
        shrinkAuthorization;
    std::unique_ptr<vdrsuite::mediatools::NativeProcessRunner>
        shrinkRunner;
    std::unique_ptr<vdrsuite::mediatools::LocalMediaShrinkAgentExecutor>
        shrinkExecutor;
    std::unique_ptr<vdrsuite::mediatools::ProtectedMediaShrinkAgentExecutor>
        protectedShrinkExecutor;
    if (enableMediaShrink)
    {
        namespace fs = std::filesystem;
        const fs::path statePath(shrinkStateDirectory);
        struct stat status{};
        if (statePath.empty() || !statePath.is_absolute() ||
            statePath.lexically_normal() != statePath ||
            shrinkStateDirectory.size() > 2048 ||
            ::lstat(statePath.c_str(), &status) != 0 ||
            !S_ISDIR(status.st_mode) || status.st_uid != ::geteuid() ||
            (status.st_mode & 0077) != 0 ||
            !vdrsuite::addons::inspectMediaToolsInstallSnapshot(
                "/").bytesVerified() ||
            std::find(config.commandTypes.begin(), config.commandTypes.end(),
                      "probe.noop") != config.commandTypes.end())
        {
            std::cerr << "HEVC opt-in requires an approved Media Tools "
                         "installation and an existing private state directory"
                      << std::endl;
            return 78;
        }
        shrinkReviews = std::make_unique<
            vdrsuite::mediatools::LocalMediaShrinkReviewStore>(statePath);
        shrinkAuthorization = std::make_unique<
            vdrsuite::agent::ReceiptRevalidatingMediaShrinkAuthorization>(
                config.commandStatePath, transport, [&]() {
                    BackendAgentCommandClientContext context;
                    if (runtimeAccess)
                    {
                        const auto& state = runtimeAccess->state();
                        context.agentId = state.agentId;
                        context.credentialSecret = state.credentialSecret;
                        context.backendId = state.backendId;
                        context.agentInstanceId = runtimeAccess->agentInstanceId();
                        context.backendGeneration = state.backendGeneration;
                    }
                    return context;
                });
        shrinkRunner = std::make_unique<
            vdrsuite::mediatools::NativeProcessRunner>(
                std::chrono::seconds(4 * 3600));
        shrinkExecutor = std::make_unique<
            vdrsuite::mediatools::LocalMediaShrinkAgentExecutor>(
                *shrinkReviews,
                [&](const std::string& type) {
                    return (type ==
                        vdrsuite::mediatools::kMediaShrinkStageCommandType ||
                        type ==
                        vdrsuite::mediatools::kMediaShrinkPublishCommandType) &&
                        vdrsuite::addons::inspectMediaToolsInstallSnapshot(
                            "/").bytesVerified();
                },
                [&](const BackendAgentCommandAssignment& assignment) {
                    return shrinkAuthorization->authority(assignment);
                },
                [&]() -> vdrsuite::mediatools::ProcessRunner& {
                    return *shrinkRunner;
                });
        // The real Agent never injects the naked C++ shrink executor:
        // the protective lock spans every stage/publish child process.
        protectedShrinkExecutor = std::make_unique<
            vdrsuite::mediatools::ProtectedMediaShrinkAgentExecutor>(
                *shrinkExecutor, *shrinkRunner, "/");
        config.mediaShrinkExecutor = protectedShrinkExecutor.get();
        for (const auto* type : {
                vdrsuite::mediatools::kMediaShrinkStageCommandType,
                vdrsuite::mediatools::kMediaShrinkPublishCommandType})
            if (std::find(config.commandTypes.begin(),
                          config.commandTypes.end(), type) ==
                config.commandTypes.end())
                config.commandTypes.emplace_back(type);
    }
    BackendAgentClientRuntime runtime(
        config,
        transport,
        {},
        [](const std::string& message) { std::cout << message << std::endl; });
    runtimeAccess = &runtime;

    if (once || rotateCredential)
    {
        if (!runtime.synchronize(reason))
        {
            std::cerr << "Backend Agent synchronization failed: " << reason << std::endl;
            return 1;
        }
        if (rotateCredential)
        {
            if (!runtime.rotateCredential(reason))
            {
                std::cerr << "Backend Agent credential rotation failed: "
                          << reason << std::endl;
                return 1;
            }
            std::cout << "Backend Agent credential rotation succeeded" << std::endl;
            return 0;
        }
        std::cout << "Backend Agent synchronization succeeded" << std::endl;
        return 0;
    }

    std::signal(SIGINT, handleSignal);
    std::signal(SIGTERM, handleSignal);
    return runtime.run([] { return StopRequested.load(); });
}
