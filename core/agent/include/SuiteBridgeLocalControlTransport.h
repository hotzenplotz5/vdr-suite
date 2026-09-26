#pragma once
#include "BackendAgentNativeProbe.h"
#include "SuiteBridgeControlPlaneProtocol.h"
#include "SuiteBridgeLiveSourceTransport.h"
#include <atomic>
#include <chrono>
#include <string>
namespace vdrsuite::agent {
struct SuiteBridgeLocalControlTransportConfig { std::string socketPath="/run/vdr/vdr-suite-control/control.sock"; std::chrono::milliseconds connectTimeout{250}; std::chrono::milliseconds ioTimeout{500}; std::chrono::milliseconds operationTimeout{1000}; };
class SuiteBridgeLocalControlTransport final:public ISuiteBridgeLiveSourceTransport,public IBackendAgentNativeProbeTransport {
public:
 explicit SuiteBridgeLocalControlTransport(SuiteBridgeLocalControlTransportConfig config={});
 SuiteBridgeCommandReply discoverNativeProbe() override;
 SuiteBridgeCommandReply executeNativeProbe(const SuiteBridgeNativeProbeRequest&) override;
 SuiteBridgeCommandReply readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&) override;
 SuiteBridgeCommandReply discoverLiveSource() override;
 SuiteBridgeCommandReply openLiveSource(const SuiteBridgeLiveSourceOpenRequest&) override;
 SuiteBridgeCommandReply closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&) override;
 SuiteBridgeCommandReply statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&) override;
private:
 static bool safeToken(const std::string&);
 SuiteBridgeCommandReply execute(control::Operation,const std::string&);
 SuiteBridgeLocalControlTransportConfig config_;
 std::atomic<std::uint64_t> nextRequestId_{1};
};
class SuiteBridgePrioritizedNativeProbeTransport final:public IBackendAgentNativeProbeTransport {
public:
 SuiteBridgePrioritizedNativeProbeTransport(IBackendAgentNativeProbeTransport& dedicated,IBackendAgentNativeProbeTransport& compatibility):dedicated_(dedicated),compatibility_(compatibility){}
 SuiteBridgeCommandReply discoverNativeProbe() override;
 SuiteBridgeCommandReply executeNativeProbe(const SuiteBridgeNativeProbeRequest&) override;
 SuiteBridgeCommandReply readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&) override;
private:
 template<class Call> SuiteBridgeCommandReply select(Call&& call){auto r=call(dedicated_);if(r.transportStatus!=SuiteBridgeTransportStatus::Unavailable)return r;return call(compatibility_);}
 IBackendAgentNativeProbeTransport& dedicated_; IBackendAgentNativeProbeTransport& compatibility_;
};

class SuiteBridgePrioritizedLiveTransport final:public ISuiteBridgeLiveSourceTransport {
public:
 SuiteBridgePrioritizedLiveTransport(ISuiteBridgeLiveSourceTransport& dedicated,ISuiteBridgeLiveSourceTransport& compatibility):dedicated_(dedicated),compatibility_(compatibility){}
 SuiteBridgeCommandReply discoverLiveSource() override;
 SuiteBridgeCommandReply openLiveSource(const SuiteBridgeLiveSourceOpenRequest&) override;
 SuiteBridgeCommandReply closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&) override;
 SuiteBridgeCommandReply statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&) override;
private:
 template<class Call> SuiteBridgeCommandReply select(Call&& call){auto r=call(dedicated_);if(r.transportStatus!=SuiteBridgeTransportStatus::Unavailable)return r;return call(compatibility_);}
 ISuiteBridgeLiveSourceTransport& dedicated_; ISuiteBridgeLiveSourceTransport& compatibility_;
};
} // namespace vdrsuite::agent
