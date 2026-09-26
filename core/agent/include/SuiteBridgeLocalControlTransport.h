#pragma once

#include "BackendAgentNativeProbe.h"
#include "ISuiteBridgeLegacyOsdInputTransport.h"
#include "ISuiteBridgeHbbtvTransport.h"
#include "ISuiteBridgeLocalTransport.h"
#include "SuiteBridgeControlPlaneProtocol.h"
#include "SuiteBridgeLiveSourceTransport.h"

#include <atomic>
#include <chrono>
#include <string>

namespace vdrsuite::agent
{

struct SuiteBridgeLocalControlTransportConfig
{
    std::string socketPath =
        "/run/vdr/vdr-suite-control/control.sock";
    std::chrono::milliseconds connectTimeout{250};
    std::chrono::milliseconds ioTimeout{500};
    std::chrono::milliseconds operationTimeout{1000};
};

class SuiteBridgeLocalControlTransport final :
    public ISuiteBridgeLiveSourceTransport,
    public IBackendAgentNativeProbeTransport,
    public ISuiteBridgeLocalTransport,
    public ISuiteBridgeLegacyOsdInputTransport,
    public ::ISuiteBridgeHbbtvTransport
{
public:
    explicit SuiteBridgeLocalControlTransport(
        SuiteBridgeLocalControlTransportConfig config = {});

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

    bool legacyOsdInputAvailable() override;
    SuiteBridgeCommandReply executeLegacyOsdInput(
        const LegacyOsdInputCommand& request,
        const std::string& requestFingerprint) override;

    SuiteBridgeCommandReply discoverNativeProbe() override;
    SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest&) override;
    SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest&) override;

    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string& channelId) override;
    SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest& request) override;
    SuiteBridgeHbbtvCommandReply readHbbtvPresentation(
        const SuiteBridgeHbbtvPresentationRequest& request) override;
    SuiteBridgeHbbtvCommandReply readHbbtvMedia(
        const SuiteBridgeHbbtvMediaRequest& request) override;

    SuiteBridgeCommandReply discoverLiveSource() override;
    SuiteBridgeCommandReply openLiveSource(
        const SuiteBridgeLiveSourceOpenRequest&) override;
    SuiteBridgeCommandReply closeLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override;
    SuiteBridgeCommandReply statusLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override;

private:
    static bool safeToken(const std::string&);
    SuiteBridgeCommandReply executeOperation(
        control::Operation,
        const std::string&);

    SuiteBridgeLocalControlTransportConfig config_;
    std::atomic<std::uint64_t> nextRequestId_{1};
};

class SuiteBridgePrioritizedLocalTransport final :
    public ISuiteBridgeLocalTransport
{
public:
    SuiteBridgePrioritizedLocalTransport(
        ISuiteBridgeLocalTransport& dedicated,
        ISuiteBridgeLocalTransport& compatibility)
        : dedicated_(dedicated),
          compatibility_(compatibility)
    {
    }

    SuiteBridgeCommandReply execute(
        SuiteBridgeLocalCommand command) override;

private:
    ISuiteBridgeLocalTransport& dedicated_;
    ISuiteBridgeLocalTransport& compatibility_;
};

class SuiteBridgePrioritizedLegacyOsdInputTransport final :
    public ISuiteBridgeLegacyOsdInputTransport
{
public:
    SuiteBridgePrioritizedLegacyOsdInputTransport(
        ISuiteBridgeLocalTransport& capabilityTransport,
        ISuiteBridgeLegacyOsdInputTransport& dedicated,
        ISuiteBridgeLegacyOsdInputTransport& compatibility)
        : capabilityTransport_(capabilityTransport),
          dedicated_(dedicated),
          compatibility_(compatibility)
    {
    }

    bool legacyOsdInputAvailable() override;
    SuiteBridgeCommandReply executeLegacyOsdInput(
        const LegacyOsdInputCommand& request,
        const std::string& requestFingerprint) override;

private:
    ISuiteBridgeLocalTransport& capabilityTransport_;
    ISuiteBridgeLegacyOsdInputTransport& dedicated_;
    ISuiteBridgeLegacyOsdInputTransport& compatibility_;
};


class SuiteBridgePrioritizedHbbtvTransport final :
    public ::ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgePrioritizedHbbtvTransport(
        ::ISuiteBridgeHbbtvTransport& dedicated,
        ::ISuiteBridgeHbbtvTransport& compatibility)
        : dedicated_(dedicated),
          compatibility_(compatibility)
    {
    }

    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string& channelId) override;
    SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest& request) override;
    SuiteBridgeHbbtvCommandReply readHbbtvPresentation(
        const SuiteBridgeHbbtvPresentationRequest& request) override;
    SuiteBridgeHbbtvCommandReply readHbbtvMedia(
        const SuiteBridgeHbbtvMediaRequest& request) override;

private:
    template<class Call>
    SuiteBridgeHbbtvCommandReply select(Call&& call)
    {
        auto reply = call(dedicated_);
        if (reply.transportStatus !=
            SuiteBridgeTransportStatus::Unavailable)
            return reply;
        return call(compatibility_);
    }

    ::ISuiteBridgeHbbtvTransport& dedicated_;
    ::ISuiteBridgeHbbtvTransport& compatibility_;
};

class SuiteBridgePrioritizedNativeProbeTransport final :
    public IBackendAgentNativeProbeTransport
{
public:
    SuiteBridgePrioritizedNativeProbeTransport(
        IBackendAgentNativeProbeTransport& dedicated,
        IBackendAgentNativeProbeTransport& compatibility)
        : dedicated_(dedicated),
          compatibility_(compatibility)
    {
    }

    SuiteBridgeCommandReply discoverNativeProbe() override;
    SuiteBridgeCommandReply executeNativeProbe(
        const SuiteBridgeNativeProbeRequest&) override;
    SuiteBridgeCommandReply readNativeProbe(
        const SuiteBridgeNativeProbeReadbackRequest&) override;

private:
    template<class Call>
    SuiteBridgeCommandReply select(Call&& call)
    {
        auto reply = call(dedicated_);
        if (reply.transportStatus !=
            SuiteBridgeTransportStatus::Unavailable)
            return reply;
        return call(compatibility_);
    }

    IBackendAgentNativeProbeTransport& dedicated_;
    IBackendAgentNativeProbeTransport& compatibility_;
};

class SuiteBridgePrioritizedLiveTransport final :
    public ISuiteBridgeLiveSourceTransport
{
public:
    SuiteBridgePrioritizedLiveTransport(
        ISuiteBridgeLiveSourceTransport& dedicated,
        ISuiteBridgeLiveSourceTransport& compatibility)
        : dedicated_(dedicated),
          compatibility_(compatibility)
    {
    }

    SuiteBridgeCommandReply discoverLiveSource() override;
    SuiteBridgeCommandReply openLiveSource(
        const SuiteBridgeLiveSourceOpenRequest&) override;
    SuiteBridgeCommandReply closeLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override;
    SuiteBridgeCommandReply statusLiveSource(
        const SuiteBridgeLiveSourceLeaseRequest&) override;

private:
    template<class Call>
    SuiteBridgeCommandReply select(Call&& call)
    {
        auto reply = call(dedicated_);
        if (reply.transportStatus !=
            SuiteBridgeTransportStatus::Unavailable)
            return reply;
        return call(compatibility_);
    }

    ISuiteBridgeLiveSourceTransport& dedicated_;
    ISuiteBridgeLiveSourceTransport& compatibility_;
};

} // namespace vdrsuite::agent
