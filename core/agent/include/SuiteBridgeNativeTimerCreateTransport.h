#pragma once

#include "BackendAgentNativeTimerCreateExecutor.h"
#include "SuiteBridgeSvdrpTransport.h"

namespace vdrsuite::agent
{

// Backend/VDR-owned prospective clock, pinned to the same provider epoch
// used by fenced native CREATE. Device-supplied clock values never enter here.
struct SuiteBridgeNativeTimerClockProjection
{
    std::int64_t start = 0;
    std::int64_t stop = 0;
    std::int32_t startUtcOffsetSeconds = 0;
    std::int32_t stopUtcOffsetSeconds = 0;
    std::string startDay;
    std::string startHhmm;
    std::string stopDay;
    std::string stopHhmm;
    std::string providerInstanceEpoch;
};

// A typed, read-only native prospect assessment. This is not a
// ProspectiveNativeTimerConflictProof and must not itself enable CREATE.
struct SuiteBridgeNativeTimerPreviewProjection
{
    bool isolatedCandidateClear = false;
    std::int64_t proposedStart = 0;
    std::int64_t proposedStop = 0;
    std::int64_t nativeObservedAt = 0;
    std::uint64_t consideredTimers = 0;
    std::uint64_t consideredDevices = 0;
    std::string providerInstanceEpoch;
    std::string specificationDigest;
    std::string evidenceRevision;
};

class SuiteBridgeNativeTimerCreateTransport final :
    public IBackendAgentNativeTimerCreateTransport
{
public:
    explicit SuiteBridgeNativeTimerCreateTransport(
        SuiteBridgeSvdrpTransportConfig config = {});

    bool discoverProvider(
        BackendAgentLocalProviderFacts& facts,
        std::string& reasonCode) override;

    bool inspectClock(
        std::int64_t start, std::int64_t stop,
        const BackendAgentLocalProviderSelection& authorizedSelection,
        SuiteBridgeNativeTimerClockProjection& projection,
        std::string& reasonCode);

    bool inspectProspectiveNativeTimerPreview(
        const BackendAgentNativeTimerCreateSpecification& specification,
        std::int64_t start, std::int64_t stop,
        const BackendAgentLocalProviderSelection& authorizedSelection,
        SuiteBridgeNativeTimerPreviewProjection& projection,
        std::string& reasonCode);

    // Same verifier can be used with the existing backend-scoped Daemon
    // transport; NEVER assume a remote Backend lives on localhost.
    static bool verifyNativePreviewReply(
        const SuiteBridgeCommandReply& reply,
        const BackendAgentNativeTimerCreateSpecification& specification,
        std::int64_t start, std::int64_t stop,
        const BackendAgentLocalProviderSelection& authorizedSelection,
        std::int64_t receivedAt,
        SuiteBridgeNativeTimerPreviewProjection& projection,
        std::string& reasonCode);

    // Reusable by the Daemon's already-scoped SuiteBridge transport without
    // constructing a second localhost transport for remote backends.
    static bool verifyNativeClockReply(
        const SuiteBridgeCommandReply& reply,
        std::int64_t start, std::int64_t stop,
        const BackendAgentLocalProviderSelection& authorizedSelection,
        SuiteBridgeNativeTimerClockProjection& projection,
        std::string& reasonCode);

    BackendAgentNativeTimerCreateTransportReply createTimer(
        const BackendAgentNativeTimerCreateTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

} // namespace vdrsuite::agent
