#pragma once

#include "BackendAgentNativeProbe.h"
#include "ISuiteBridgeLocalTransport.h"
#include "ISuiteBridgeArtworkTransport.h"
#include "ISuiteBridgeEpgTypeSnapshotTransport.h"
#include "ISuiteBridgeMetadataTransport.h"
#include "ISuiteBridgeRecordingMetadataTransport.h"
#include "SuiteBridgeLiveSourceTransport.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>

namespace vdrsuite::agent
{

struct BackendAgentNativeTimerCreateTransportRequest;
struct BackendAgentNativeTimerDeleteTransportRequest;
struct BackendAgentNativeTimerModifyTransportRequest;
enum class BackendAgentNativeTimerModifyKind;

struct SuiteBridgeSvdrpTransportConfig
{
    std::string host = "127.0.0.1";
    int port = 6419;
    std::chrono::milliseconds connectTimeout{1000};
    std::chrono::milliseconds ioTimeout{1000};
    std::chrono::milliseconds operationTimeout{3000};
};

class SuiteBridgeSvdrpTransport final :
    public ISuiteBridgeLocalTransport,
    public ::ISuiteBridgeArtworkTransport,
    public ::ISuiteBridgeEpgTypeSnapshotTransport,
    public ::ISuiteBridgeMetadataTransport,
    public ::ISuiteBridgeRecordingMetadataTransport,
    public IBackendAgentNativeProbeTransport,
    public ISuiteBridgeLiveSourceTransport
{
public:
    static constexpr std::size_t MaximumGreetingBytes = 1024;
    static constexpr std::size_t MaximumReplyBytes = 131072;
    static constexpr std::size_t MaximumReplyLines = 64;

    explicit SuiteBridgeSvdrpTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

    ::SuiteBridgeArtworkCommandReply requestArtwork(
        const std::string& channelId,
        const std::string& eventId) override;

    ::SuiteBridgeEpgTypeSnapshotTransportPage requestEpgTypeSnapshot(
        std::int64_t fromTime,
        std::int64_t untilTime,
        std::uint64_t offset,
        std::size_t limit) override;

    ::SuiteBridgeMetadataCommandReply requestMetadata(
        const std::string& channelId,
        const std::string& eventId) override;

    ::SuiteBridgeRecordingMetadataCommandReply requestRecordingMetadata(
        const std::string& recordingKey) override;

    SuiteBridgeCommandReply discoverNativeProbe() override
    {
        return executeRequest("PLUG suitebridge NCAP 1\r\n");
    }

    SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest& request) override
    {
        if (!safeNativeToken(request.commandId) ||
            !safeNativeToken(request.requestFingerprint) ||
            !safeNativeToken(request.operationId) ||
            !safeNativeToken(request.jobId) ||
            !safeNativeToken(request.attemptId) || request.claimEpoch == 0 ||
            !safeNativeToken(request.backendId) ||
            !safeNativeToken(request.agentId) ||
            !safeNativeToken(request.agentInstanceId) ||
            request.backendGeneration == 0 ||
            !safeNativeToken(request.pluginInstanceEpoch) ||
            !safeNativeToken(request.probeNonce))
        {
            return {SuiteBridgeTransportStatus::Failed, 0, {},
                "invalid typed native probe request"};
        }
        std::ostringstream wire;
        wire << "PLUG suitebridge NPROBE EXEC vdr-suite-native/1 "
             << "vdr.native.probe 1 "
             << request.commandId << ' ' << request.requestFingerprint << ' '
             << request.operationId << ' ' << request.jobId << ' '
             << request.attemptId << ' ' << request.claimEpoch << ' '
             << request.backendId << ' ' << request.agentId << ' '
             << request.agentInstanceId << ' ' << request.backendGeneration << ' '
             << request.pluginInstanceEpoch << " 1 " << request.probeNonce
             << "\r\n";
        return executeRequest(wire.str());
    }

    SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest& request) override
    {
        if (!safeNativeToken(request.commandId) ||
            !safeNativeToken(request.requestFingerprint) ||
            !safeNativeToken(request.pluginInstanceEpoch) ||
            request.nativeExecutionSequence == 0)
        {
            return {SuiteBridgeTransportStatus::Failed, 0, {},
                "invalid typed native probe readback request"};
        }
        std::ostringstream wire;
        wire << "PLUG suitebridge NPROBE READ 1 "
             << request.commandId << ' ' << request.requestFingerprint << ' '
             << request.pluginInstanceEpoch << ' '
             << request.nativeExecutionSequence << "\r\n";
        return executeRequest(wire.str());
    }

    SuiteBridgeCommandReply discoverLiveSource() override
    {
        return executeRequest("PLUG suitebridge NLCAP 1\r\n");
    }

    SuiteBridgeCommandReply openLiveSource(
        const SuiteBridgeLiveSourceOpenRequest& request) override
    {
        if (!safeNativeToken(request.leaseId) ||
            !safeNativeToken(request.channelId) ||
            !safeNativeToken(request.pluginInstanceEpoch))
        {
            return {SuiteBridgeTransportStatus::Failed, 0, {},
                "invalid typed live source open request"};
        }
        std::ostringstream wire;
        wire << "PLUG suitebridge NLIVE OPEN 1 "
             << request.leaseId << ' ' << request.channelId << ' '
             << request.pluginInstanceEpoch << "\r\n";
        return executeRequest(wire.str());
    }

    SuiteBridgeCommandReply closeLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest& request) override
    {
        return executeLiveLeaseRequest("CLOSE", request);
    }

    SuiteBridgeCommandReply statusLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest& request) override
    {
        return executeLiveLeaseRequest("STATUS", request);
    }

    SuiteBridgeCommandReply discoverNativeTimerCreateContract();
    SuiteBridgeCommandReply executeNativeTimerCreateContract(
        const BackendAgentNativeTimerCreateTransportRequest& request);

    SuiteBridgeCommandReply discoverNativeTimerDeleteContract();
    SuiteBridgeCommandReply executeNativeTimerDeleteContract(
        const BackendAgentNativeTimerDeleteTransportRequest& request);

    SuiteBridgeCommandReply discoverNativeTimerModifyContract(
        BackendAgentNativeTimerModifyKind kind);
    SuiteBridgeCommandReply executeNativeTimerModifyContract(
        const BackendAgentNativeTimerModifyTransportRequest& request);

private:
    static bool safeNativeToken(const std::string& value)
    {
        return !value.empty() && value.size() <= 128 &&
            std::all_of(value.begin(), value.end(), [](unsigned char character) {
                return std::isalnum(character) != 0 || character == '-' ||
                    character == '_' || character == '.' || character == ':';
            });
    }

    SuiteBridgeCommandReply executeLiveLeaseRequest(
        const char* operation,
        const SuiteBridgeLiveSourceLeaseRequest& request)
    {
        if (operation == nullptr ||
            !safeNativeToken(request.leaseId) ||
            !safeNativeToken(request.pluginInstanceEpoch))
        {
            return {SuiteBridgeTransportStatus::Failed, 0, {},
                "invalid typed live source lease request"};
        }
        std::ostringstream wire;
        wire << "PLUG suitebridge NLIVE " << operation << " 1 "
             << request.leaseId << ' ' << request.pluginInstanceEpoch
             << "\r\n";
        return executeRequest(wire.str());
    }

    SuiteBridgeCommandReply executeRequest(
        const std::string& requestText);

    SuiteBridgeSvdrpTransportConfig config_;
};

}
