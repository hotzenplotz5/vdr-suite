#pragma once

#include "BackendAgentNativeProbe.h"
#include "ISuiteBridgeEpgArtworkTransport.h"
#include "ISuiteBridgeEpgMetadataTransport.h"
#include "ISuiteBridgeEpgTypeSnapshotTransport.h"
#include "ISuiteBridgeLocalTransport.h"
#include "ISuiteBridgeRecordingMetadataTransport.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeSvdrpTransportConfig
{
    std::string host = "127.0.0.1";
    std::uint16_t port = 6419;
    std::chrono::milliseconds connectTimeout{1000};
    std::chrono::milliseconds ioTimeout{1000};
    std::chrono::milliseconds operationTimeout{2500};
};

class SuiteBridgeSvdrpTransport final
    : public ISuiteBridgeLocalTransport,
      public ISuiteBridgeEpgArtworkTransport,
      public ISuiteBridgeEpgMetadataTransport,
      public ISuiteBridgeEpgTypeSnapshotTransport,
      public ISuiteBridgeRecordingMetadataTransport,
      public IBackendAgentNativeProbeTransport
{
public:
    explicit SuiteBridgeSvdrpTransport(
        SuiteBridgeSvdrpTransportConfig config);

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

    SuiteBridgeEpgArtworkReply fetchEpgArtwork(
        const SuiteBridgeEpgArtworkRequest& request) override;

    SuiteBridgeEpgMetadataReply fetchEpgMetadata(
        const SuiteBridgeEpgMetadataRequest& request) override;

    SuiteBridgeEpgTypeSnapshotReply fetchEpgTypeSnapshot(
        const SuiteBridgeEpgTypeSnapshotRequest& request) override;

    SuiteBridgeRecordingMetadataReply fetchRecordingMetadata(
        const SuiteBridgeRecordingMetadataRequest& request) override;

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

private:
    static bool safeNativeToken(const std::string& value)
    {
        return !value.empty() && value.size() <= 128 &&
            std::all_of(value.begin(), value.end(), [](unsigned char character) {
                return std::isalnum(character) != 0 || character == '-' ||
                    character == '_' || character == '.' || character == ':';
            });
    }

    SuiteBridgeCommandReply executeRequest(
        const std::string& request);

    SuiteBridgeSvdrpTransportConfig config_;
};

}
