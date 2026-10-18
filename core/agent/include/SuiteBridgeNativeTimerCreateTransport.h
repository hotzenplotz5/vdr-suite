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

    BackendAgentNativeTimerCreateTransportReply createTimer(
        const BackendAgentNativeTimerCreateTransportRequest& request) override;

private:
    SuiteBridgeSvdrpTransport transport_;
};

} // namespace vdrsuite::agent
