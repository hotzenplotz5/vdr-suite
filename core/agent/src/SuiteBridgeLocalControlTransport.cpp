#include "SuiteBridgeLocalControlTransport.h"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <utility>
#include <vector>
namespace vdrsuite::agent {
namespace {
using Clock=std::chrono::steady_clock; using TimePoint=Clock::time_point;
class ScopedFd{int fd_;public:explicit ScopedFd(int fd=-1):fd_(fd){}~ScopedFd(){if(fd_>=0)close(fd_);}int get()const{return fd_;}bool valid()const{return fd_>=0;}};
int timeoutMs(TimePoint d){auto n=Clock::now();if(n>=d)return 0;auto u=std::chrono::duration_cast<std::chrono::microseconds>(d-n).count();return static_cast<int>(std::min<long long>(INT_MAX,std::max<long long>(1,(u+999)/1000)));}
bool waitFor(int fd,short events,TimePoint d){for(;;){pollfd p{};p.fd=fd;p.events=events;int t=timeoutMs(d);if(t<=0)return false;int r=poll(&p,1,t);if(r==0)return false;if(r<0){if(errno==EINTR)continue;return false;}if(p.revents&events)return true;if(p.revents&(POLLERR|POLLHUP|POLLNVAL))return false;}}
SuiteBridgeCommandReply fail(SuiteBridgeTransportStatus s,std::string d){SuiteBridgeCommandReply r;r.transportStatus=s;r.diagnostic=std::move(d);return r;}
std::string diagnostic(control::Result r){switch(r){case control::Result::Overloaded:return "queue_full";case control::Result::DeadlineExpired:return "deadline_expired_before_execution";case control::Result::ProviderUnavailable:return "provider_unavailable";case control::Result::ProtocolMismatch:return "protocol_mismatch";case control::Result::PeerRejected:return "peer_rejected";case control::Result::InternalFailure:return "control_plane_internal_failure";case control::Result::StaleRejected:return "stale_rejected";case control::Result::NativeRejected:return "native_rejected";case control::Result::Success:return {};}return "unknown_control_plane_result";}
}
SuiteBridgeLocalControlTransport::SuiteBridgeLocalControlTransport(SuiteBridgeLocalControlTransportConfig c):config_(std::move(c)){}
bool SuiteBridgeLocalControlTransport::safeToken(const std::string&v){return !v.empty()&&v.size()<=128&&std::all_of(v.begin(),v.end(),[](unsigned char c){return std::isalnum(c)!=0||c=='-'||c=='_'||c=='.'||c==':';});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::discoverNativeProbe(){return execute(control::Operation::NativeProbeCapability,{});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::executeNativeProbe(const SuiteBridgeNativeProbeRequest&r){
 if(!safeToken(r.commandId)||!safeToken(r.requestFingerprint)||!safeToken(r.operationId)||!safeToken(r.jobId)||!safeToken(r.attemptId)||r.claimEpoch==0||!safeToken(r.backendId)||!safeToken(r.agentId)||!safeToken(r.agentInstanceId)||r.backendGeneration==0||!safeToken(r.pluginInstanceEpoch)||!safeToken(r.probeNonce))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed native probe request");
 return execute(control::Operation::NativeProbeExecute,r.commandId+"\n"+r.requestFingerprint+"\n"+r.operationId+"\n"+r.jobId+"\n"+r.attemptId+"\n"+std::to_string(r.claimEpoch)+"\n"+r.backendId+"\n"+r.agentId+"\n"+r.agentInstanceId+"\n"+std::to_string(r.backendGeneration)+"\n"+r.pluginInstanceEpoch+"\n"+r.probeNonce);
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&r){
 if(!safeToken(r.commandId)||!safeToken(r.requestFingerprint)||!safeToken(r.pluginInstanceEpoch)||r.nativeExecutionSequence==0)return fail(SuiteBridgeTransportStatus::Failed,"invalid typed native probe readback request");
 return execute(control::Operation::NativeProbeReadback,r.commandId+"\n"+r.requestFingerprint+"\n"+r.pluginInstanceEpoch+"\n"+std::to_string(r.nativeExecutionSequence));
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::discoverLiveSource(){return execute(control::Operation::LiveCapability,{});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::openLiveSource(const SuiteBridgeLiveSourceOpenRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.channelId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source open request");return execute(control::Operation::LiveOpen,r.leaseId+"\n"+r.channelId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source close request");return execute(control::Operation::LiveClose,r.leaseId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source status request");return execute(control::Operation::LiveStatus,r.leaseId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::execute(control::Operation op,const std::string&payload){
 if(config_.socketPath.empty()||config_.socketPath.size()>=sizeof(sockaddr_un::sun_path))return fail(SuiteBridgeTransportStatus::Failed,"invalid local control socket path");
 auto operationDeadline=Clock::now()+config_.operationTimeout; ScopedFd fd(socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0)); if(!fd.valid())return fail(SuiteBridgeTransportStatus::Unavailable,"local control socket unavailable");
 int flags=fcntl(fd.get(),F_GETFL,0);if(flags<0||fcntl(fd.get(),F_SETFL,flags|O_NONBLOCK)!=0)return fail(SuiteBridgeTransportStatus::Failed,"local control socket configuration failed");
 sockaddr_un a{};a.sun_family=AF_UNIX;std::copy(config_.socketPath.begin(),config_.socketPath.end(),a.sun_path);a.sun_path[config_.socketPath.size()]='\0';
 if(connect(fd.get(),reinterpret_cast<const sockaddr*>(&a),sizeof(a))!=0){if(errno!=EINPROGRESS&&errno!=EAGAIN){
     const bool unavailable=errno==ENOENT||errno==ECONNREFUSED;
     return fail(unavailable?SuiteBridgeTransportStatus::Unavailable:SuiteBridgeTransportStatus::Failed,
         unavailable?"local control endpoint unavailable":"local control endpoint connect failed");
   }auto d=std::min(operationDeadline,Clock::now()+config_.connectTimeout);if(!waitFor(fd.get(),POLLOUT,d))return fail(SuiteBridgeTransportStatus::Unavailable,"local control endpoint unavailable");int e=0;socklen_t z=sizeof(e);if(getsockopt(fd.get(),SOL_SOCKET,SO_ERROR,&e,&z)!=0||e!=0)return fail(SuiteBridgeTransportStatus::Unavailable,"local control endpoint unavailable");}
 control::Request q;q.operation=op;q.requestId=nextRequestId_.fetch_add(1);if(q.requestId==0)q.requestId=nextRequestId_.fetch_add(1);q.deadlineNanoseconds=control::monotonicNowNanoseconds()+static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(config_.operationTimeout).count());q.payload=payload;
 std::vector<std::uint8_t> frame;if(!control::encodeRequest(q,frame))return fail(SuiteBridgeTransportStatus::Failed,"local control request encoding failed");
 if(!waitFor(fd.get(),POLLOUT,std::min(operationDeadline,Clock::now()+config_.ioTimeout)))return fail(SuiteBridgeTransportStatus::Failed,"local control request send timed out");
 ssize_t sent=send(fd.get(),frame.data(),frame.size(),MSG_NOSIGNAL);if(sent!=static_cast<ssize_t>(frame.size()))return fail(SuiteBridgeTransportStatus::Failed,"local control request send failed");
 if(!waitFor(fd.get(),POLLIN,std::min(operationDeadline,Clock::now()+config_.ioTimeout)))return fail(SuiteBridgeTransportStatus::Timeout,"local control response timed out");
 std::vector<std::uint8_t>b(control::MaximumResponseFrameBytes+1);ssize_t n=recv(fd.get(),b.data(),b.size(),MSG_TRUNC);if(n<=0||static_cast<std::size_t>(n)>control::MaximumResponseFrameBytes)return fail(SuiteBridgeTransportStatus::Failed,"local control response framing failed");b.resize(static_cast<std::size_t>(n));control::Response p;std::string why;if(!control::decodeResponse(b,p,why)||p.requestId!=q.requestId)return fail(SuiteBridgeTransportStatus::Failed,"local control response protocol mismatch");
 if(p.result!=control::Result::Success&&p.result!=control::Result::NativeRejected&&p.result!=control::Result::StaleRejected)return fail(p.result==control::Result::DeadlineExpired?SuiteBridgeTransportStatus::Timeout:SuiteBridgeTransportStatus::Failed,diagnostic(p.result));
 SuiteBridgeCommandReply r;r.transportStatus=SuiteBridgeTransportStatus::Success;r.replyCode=p.replyCode;r.payload=std::move(p.payload);r.diagnostic=diagnostic(p.result);return r;
}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::discoverLiveSource(){return select([](auto&t){return t.discoverLiveSource();});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::openLiveSource(const SuiteBridgeLiveSourceOpenRequest&r){return select([&](auto&t){return t.openLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){return select([&](auto&t){return t.closeLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){return select([&](auto&t){return t.statusLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::discoverNativeProbe(){return select([](auto&t){return t.discoverNativeProbe();});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::executeNativeProbe(const SuiteBridgeNativeProbeRequest&r){return select([&](auto&t){return t.executeNativeProbe(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&r){return select([&](auto&t){return t.readNativeProbe(r);});}
} // namespace vdrsuite::agent
