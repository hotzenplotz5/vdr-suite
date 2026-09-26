#include "SuiteBridgeLocalControlTransport.h"
#include "SuiteBridgeHandshakeService.h"
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

const char* hbbtvRuntimeOperationName(SuiteBridgeHbbtvRuntimeOperation operation)
{
 switch(operation){
  case SuiteBridgeHbbtvRuntimeOperation::Launch:return "LAUNCH";
  case SuiteBridgeHbbtvRuntimeOperation::Status:return "STATUS";
  case SuiteBridgeHbbtvRuntimeOperation::Input:return "INPUT";
  case SuiteBridgeHbbtvRuntimeOperation::Close:return "CLOSE";
 }
 return nullptr;
}
const char* hbbtvInputActionName(SuiteBridgeHbbtvInputAction action)
{
 switch(action){
  case SuiteBridgeHbbtvInputAction::None:return nullptr;
  case SuiteBridgeHbbtvInputAction::Up:return "UP";
  case SuiteBridgeHbbtvInputAction::Down:return "DOWN";
  case SuiteBridgeHbbtvInputAction::Left:return "LEFT";
  case SuiteBridgeHbbtvInputAction::Right:return "RIGHT";
  case SuiteBridgeHbbtvInputAction::Ok:return "OK";
  case SuiteBridgeHbbtvInputAction::Back:return "BACK";
  case SuiteBridgeHbbtvInputAction::Red:return "RED";
  case SuiteBridgeHbbtvInputAction::Green:return "GREEN";
  case SuiteBridgeHbbtvInputAction::Yellow:return "YELLOW";
  case SuiteBridgeHbbtvInputAction::Blue:return "BLUE";
  case SuiteBridgeHbbtvInputAction::Digit0:return "0";
  case SuiteBridgeHbbtvInputAction::Digit1:return "1";
  case SuiteBridgeHbbtvInputAction::Digit2:return "2";
  case SuiteBridgeHbbtvInputAction::Digit3:return "3";
  case SuiteBridgeHbbtvInputAction::Digit4:return "4";
  case SuiteBridgeHbbtvInputAction::Digit5:return "5";
  case SuiteBridgeHbbtvInputAction::Digit6:return "6";
  case SuiteBridgeHbbtvInputAction::Digit7:return "7";
  case SuiteBridgeHbbtvInputAction::Digit8:return "8";
  case SuiteBridgeHbbtvInputAction::Digit9:return "9";
  case SuiteBridgeHbbtvInputAction::Play:return "PLAY";
  case SuiteBridgeHbbtvInputAction::Pause:return "PAUSE";
  case SuiteBridgeHbbtvInputAction::Stop:return "STOP";
  case SuiteBridgeHbbtvInputAction::FastForward:return "FAST_FORWARD";
  case SuiteBridgeHbbtvInputAction::Rewind:return "REWIND";
 }
 return nullptr;
}
SuiteBridgeHbbtvCommandReply hbbtvReply(const SuiteBridgeCommandReply& reply)
{
 SuiteBridgeHbbtvCommandReply result;
 result.replyCode=reply.replyCode;
 result.payload=reply.payload;
 switch(reply.transportStatus){
  case SuiteBridgeTransportStatus::Success:
   result.transportStatus=SuiteBridgeHbbtvTransportStatus::Success;break;
  case SuiteBridgeTransportStatus::Unavailable:
   result.transportStatus=SuiteBridgeHbbtvTransportStatus::Unavailable;break;
  case SuiteBridgeTransportStatus::Timeout:
   result.transportStatus=SuiteBridgeHbbtvTransportStatus::Timeout;break;
  case SuiteBridgeTransportStatus::Failed:
   result.transportStatus=SuiteBridgeHbbtvTransportStatus::Failed;break;
 }
 result.transportSucceeded=
     result.transportStatus==SuiteBridgeHbbtvTransportStatus::Success &&
     result.replyCode==250;
 return result;
}

SuiteBridgeTeletextCommandReply teletextReply(const SuiteBridgeCommandReply& reply)
{
 SuiteBridgeTeletextCommandReply result;
 result.replyCode=reply.replyCode;
 result.payload=reply.payload;
 switch(reply.transportStatus){
  case SuiteBridgeTransportStatus::Success:
   result.transportStatus=SuiteBridgeTeletextTransportStatus::Success;break;
  case SuiteBridgeTransportStatus::Unavailable:
   result.transportStatus=SuiteBridgeTeletextTransportStatus::Unavailable;break;
  case SuiteBridgeTransportStatus::Timeout:
   result.transportStatus=SuiteBridgeTeletextTransportStatus::Timeout;break;
  case SuiteBridgeTransportStatus::Failed:
   result.transportStatus=SuiteBridgeTeletextTransportStatus::Failed;break;
 }
 result.transportSucceeded=
     result.transportStatus==SuiteBridgeTeletextTransportStatus::Success &&
     result.replyCode==250;
 return result;
}

template<class Reply>
Reply readReply(const SuiteBridgeCommandReply& reply)
{
 Reply result;
 result.replyCode=reply.replyCode;
 result.payload=reply.payload;
 switch(reply.transportStatus){
  case SuiteBridgeTransportStatus::Success:
   result.transportStatus=SuiteBridgeReadTransportStatus::Success;break;
  case SuiteBridgeTransportStatus::Unavailable:
   result.transportStatus=SuiteBridgeReadTransportStatus::Unavailable;break;
  case SuiteBridgeTransportStatus::Timeout:
   result.transportStatus=SuiteBridgeReadTransportStatus::Timeout;break;
  case SuiteBridgeTransportStatus::Failed:
   result.transportStatus=SuiteBridgeReadTransportStatus::Failed;break;
 }
 result.transportSucceeded=
     result.transportStatus==SuiteBridgeReadTransportStatus::Success &&
     result.replyCode==250;
 return result;
}
}
SuiteBridgeLocalControlTransport::SuiteBridgeLocalControlTransport(SuiteBridgeLocalControlTransportConfig c):config_(std::move(c)){}
bool SuiteBridgeLocalControlTransport::safeToken(const std::string&v){return !v.empty()&&v.size()<=128&&std::all_of(v.begin(),v.end(),[](unsigned char c){return std::isalnum(c)!=0||c=='-'||c=='_'||c=='.'||c==':';});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::discoverNativeProbe(){return executeOperation(control::Operation::NativeProbeCapability,{});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::executeNativeProbe(const SuiteBridgeNativeProbeRequest&r){
 if(!safeToken(r.commandId)||!safeToken(r.requestFingerprint)||!safeToken(r.operationId)||!safeToken(r.jobId)||!safeToken(r.attemptId)||r.claimEpoch==0||!safeToken(r.backendId)||!safeToken(r.agentId)||!safeToken(r.agentInstanceId)||r.backendGeneration==0||!safeToken(r.pluginInstanceEpoch)||!safeToken(r.probeNonce))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed native probe request");
 return executeOperation(control::Operation::NativeProbeExecute,r.commandId+"\n"+r.requestFingerprint+"\n"+r.operationId+"\n"+r.jobId+"\n"+r.attemptId+"\n"+std::to_string(r.claimEpoch)+"\n"+r.backendId+"\n"+r.agentId+"\n"+r.agentInstanceId+"\n"+std::to_string(r.backendGeneration)+"\n"+r.pluginInstanceEpoch+"\n"+r.probeNonce);
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&r){
 if(!safeToken(r.commandId)||!safeToken(r.requestFingerprint)||!safeToken(r.pluginInstanceEpoch)||r.nativeExecutionSequence==0)return fail(SuiteBridgeTransportStatus::Failed,"invalid typed native probe readback request");
 return executeOperation(control::Operation::NativeProbeReadback,r.commandId+"\n"+r.requestFingerprint+"\n"+r.pluginInstanceEpoch+"\n"+std::to_string(r.nativeExecutionSequence));
}
SuiteBridgeHbbtvCommandReply SuiteBridgeLocalControlTransport::discoverHbbtv(const std::string& channelId){
 if(!safeToken(channelId)||channelId.size()>=64)return {};
 return hbbtvReply(executeOperation(control::Operation::HbbtvDiscovery,"1 "+channelId));
}
SuiteBridgeHbbtvCommandReply SuiteBridgeLocalControlTransport::controlHbbtv(const SuiteBridgeHbbtvRuntimeRequest&r){
 const char* operation=hbbtvRuntimeOperationName(r.operation);
 if(operation==nullptr||!safeToken(r.sessionId)||r.sessionId.size()>=128||!safeToken(r.channelId)||r.channelId.size()>=64||r.applicationId==0||r.descriptorRevision==0)return {};
 const bool input=r.operation==SuiteBridgeHbbtvRuntimeOperation::Input;
 const char* action=hbbtvInputActionName(r.inputAction);
 if((input&&action==nullptr)||(!input&&r.inputAction!=SuiteBridgeHbbtvInputAction::None))return {};
 std::string payload=std::string(operation)+" 1 "+r.sessionId+" "+r.channelId+" "+std::to_string(r.applicationId)+" "+std::to_string(r.descriptorRevision);
 if(input)payload+=" "+std::string(action);
 return hbbtvReply(executeOperation(control::Operation::HbbtvRuntime,payload));
}
SuiteBridgeHbbtvCommandReply SuiteBridgeLocalControlTransport::readHbbtvPresentation(const SuiteBridgeHbbtvPresentationRequest&r){
 if(!safeToken(r.sessionId)||r.sessionId.size()>=128)return {};
 if(r.operation==SuiteBridgeHbbtvPresentationOperation::Meta){
  if(r.frameRevision!=0||r.offset!=0)return {};
  return hbbtvReply(executeOperation(control::Operation::HbbtvPresentation,"META 1 "+r.sessionId));
 }
 if(r.operation==SuiteBridgeHbbtvPresentationOperation::Chunk){
  if(r.frameRevision==0)return {};
  return hbbtvReply(executeOperation(control::Operation::HbbtvPresentation,"CHUNK 1 "+r.sessionId+" "+std::to_string(r.frameRevision)+" "+std::to_string(r.offset)));
 }
 return {};
}
SuiteBridgeHbbtvCommandReply SuiteBridgeLocalControlTransport::readHbbtvMedia(const SuiteBridgeHbbtvMediaRequest&r){
 if(!safeToken(r.sessionId)||r.sessionId.size()>=128)return {};
 return hbbtvReply(executeOperation(control::Operation::HbbtvMedia,"1 "+r.sessionId));
}
SuiteBridgeTeletextCommandReply SuiteBridgeLocalControlTransport::discoverTeletext(){
 return teletextReply(executeOperation(control::Operation::TeletextCapability,"1"));
}
SuiteBridgeTeletextCommandReply SuiteBridgeLocalControlTransport::requestTeletextPage(const SuiteBridgeTeletextPageRequest&r){
 if(!safeToken(r.channelId)||r.channelId.size()>=64||r.pageNumber<100||r.pageNumber>899||(!r.automaticSubpage&&r.subpageCode==0xffffU))return {};
 std::string payload="1 "+r.channelId+" "+std::to_string(r.pageNumber)+" ";
 payload+=r.automaticSubpage?"auto":std::to_string(r.subpageCode);
 return teletextReply(executeOperation(control::Operation::TeletextPage,payload));
}
SuiteBridgeArtworkCommandReply SuiteBridgeLocalControlTransport::requestArtwork(const std::string& channelId,const std::string& eventId){
 if(!safeToken(channelId)||!safeToken(eventId))return {};
 return readReply<SuiteBridgeArtworkCommandReply>(
     executeOperation(control::Operation::EpgArtwork,channelId+" "+eventId));
}
SuiteBridgeMetadataCommandReply SuiteBridgeLocalControlTransport::requestMetadata(const std::string& channelId,const std::string& eventId){
 if(!safeToken(channelId)||!safeToken(eventId))return {};
 return readReply<SuiteBridgeMetadataCommandReply>(
     executeOperation(control::Operation::EpgMetadata,channelId+" "+eventId));
}
SuiteBridgeRecordingMetadataCommandReply SuiteBridgeLocalControlTransport::requestRecordingMetadata(const std::string& recordingKey){
 if(recordingKey.size()!=64||!std::all_of(recordingKey.begin(),recordingKey.end(),[](unsigned char ch){return std::isxdigit(ch)!=0;}))return {};
 return readReply<SuiteBridgeRecordingMetadataCommandReply>(
     executeOperation(control::Operation::RecordingMetadata,recordingKey));
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::discoverLiveSource(){return executeOperation(control::Operation::LiveCapability,{});}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::openLiveSource(const SuiteBridgeLiveSourceOpenRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.channelId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source open request");return executeOperation(control::Operation::LiveOpen,r.leaseId+"\n"+r.channelId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source close request");return executeOperation(control::Operation::LiveClose,r.leaseId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){if(!safeToken(r.leaseId)||!safeToken(r.pluginInstanceEpoch))return fail(SuiteBridgeTransportStatus::Failed,"invalid typed live source status request");return executeOperation(control::Operation::LiveStatus,r.leaseId+"\n"+r.pluginInstanceEpoch);}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::execute(
    SuiteBridgeLocalCommand command)
{
 switch(command){
  case SuiteBridgeLocalCommand::DiscoverSchema1:
   return executeOperation(control::Operation::CapabilityDiscovery,{});
  case SuiteBridgeLocalCommand::OsdSnapshot:
   return executeOperation(control::Operation::OsdSnapshot,{});
  case SuiteBridgeLocalCommand::Snapshot:
   return fail(
       SuiteBridgeTransportStatus::Unavailable,
       "local control operation not migrated");
 }
 return fail(SuiteBridgeTransportStatus::Failed,"unknown local command");
}
bool SuiteBridgeLocalControlTransport::legacyOsdInputAvailable()
{
 SuiteBridgeHandshakeService handshake(*this);
 const auto result=handshake.discover();
 return result.compatible() &&
     result.discovery.capabilityAvailable("osd.control");
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::executeLegacyOsdInput(
    const LegacyOsdInputCommand& request,
    const std::string& requestFingerprint)
{
 if(!legacyOsdInputCommandValid(request) ||
    !safeToken(requestFingerprint))
  return fail(
      SuiteBridgeTransportStatus::Failed,
      "invalid typed Legacy OSD input request");
 return executeOperation(
     control::Operation::OsdInput,
     request.inputCommandId+"\n"+
     requestFingerprint+"\n"+
     std::to_string(request.backendGeneration)+"\n"+
     request.osdSurfaceId+"\n"+
     request.osdEpoch+"\n"+
     request.controllerLeaseId+"\n"+
     std::to_string(request.controllerLeaseEpoch)+"\n"+
     std::to_string(request.leaseRevision)+"\n"+
     legacyOsdInputActionName(request.action)+"\n"+
     std::to_string(request.deadline));
}
SuiteBridgeCommandReply SuiteBridgeLocalControlTransport::executeOperation(control::Operation op,const std::string&payload){
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
SuiteBridgeCommandReply SuiteBridgePrioritizedLocalTransport::execute(
    SuiteBridgeLocalCommand command)
{
 auto reply=dedicated_.execute(command);
 if(reply.transportStatus!=SuiteBridgeTransportStatus::Unavailable)
  return reply;
 return compatibility_.execute(command);
}
bool SuiteBridgePrioritizedLegacyOsdInputTransport::legacyOsdInputAvailable()
{
 SuiteBridgeHandshakeService handshake(capabilityTransport_);
 const auto result=handshake.discover();
 return result.compatible() &&
     result.discovery.capabilityAvailable("osd.control");
}
SuiteBridgeCommandReply SuiteBridgePrioritizedLegacyOsdInputTransport::executeLegacyOsdInput(
    const LegacyOsdInputCommand& request,
    const std::string& requestFingerprint)
{
 auto reply=dedicated_.executeLegacyOsdInput(
     request,requestFingerprint);
 if(reply.transportStatus!=SuiteBridgeTransportStatus::Unavailable)
  return reply;
 return compatibility_.executeLegacyOsdInput(
     request,requestFingerprint);
}
SuiteBridgeHbbtvCommandReply SuiteBridgePrioritizedHbbtvTransport::discoverHbbtv(const std::string& channelId){return select([&](auto&t){return t.discoverHbbtv(channelId);});}
SuiteBridgeHbbtvCommandReply SuiteBridgePrioritizedHbbtvTransport::controlHbbtv(const SuiteBridgeHbbtvRuntimeRequest&r){return select([&](auto&t){return t.controlHbbtv(r);});}
SuiteBridgeHbbtvCommandReply SuiteBridgePrioritizedHbbtvTransport::readHbbtvPresentation(const SuiteBridgeHbbtvPresentationRequest&r){return select([&](auto&t){return t.readHbbtvPresentation(r);});}
SuiteBridgeHbbtvCommandReply SuiteBridgePrioritizedHbbtvTransport::readHbbtvMedia(const SuiteBridgeHbbtvMediaRequest&r){return select([&](auto&t){return t.readHbbtvMedia(r);});}
SuiteBridgeTeletextCommandReply SuiteBridgePrioritizedTeletextTransport::discoverTeletext(){return select([](auto&t){return t.discoverTeletext();});}
SuiteBridgeTeletextCommandReply SuiteBridgePrioritizedTeletextTransport::requestTeletextPage(const SuiteBridgeTeletextPageRequest&r){return select([&](auto&t){return t.requestTeletextPage(r);});}
SuiteBridgeArtworkCommandReply SuiteBridgePrioritizedArtworkTransport::requestArtwork(const std::string& channelId,const std::string& eventId){
 auto reply=dedicated_.requestArtwork(channelId,eventId);
 if(reply.transportStatus!=SuiteBridgeReadTransportStatus::Unavailable)return reply;
 return compatibility_.requestArtwork(channelId,eventId);
}
SuiteBridgeMetadataCommandReply SuiteBridgePrioritizedMetadataTransport::requestMetadata(const std::string& channelId,const std::string& eventId){
 auto reply=dedicated_.requestMetadata(channelId,eventId);
 if(reply.transportStatus!=SuiteBridgeReadTransportStatus::Unavailable)return reply;
 return compatibility_.requestMetadata(channelId,eventId);
}
SuiteBridgeRecordingMetadataCommandReply SuiteBridgePrioritizedRecordingMetadataTransport::requestRecordingMetadata(const std::string& recordingKey){
 auto reply=dedicated_.requestRecordingMetadata(recordingKey);
 if(reply.transportStatus!=SuiteBridgeReadTransportStatus::Unavailable)return reply;
 return compatibility_.requestRecordingMetadata(recordingKey);
}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::discoverLiveSource(){return select([](auto&t){return t.discoverLiveSource();});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::openLiveSource(const SuiteBridgeLiveSourceOpenRequest&r){return select([&](auto&t){return t.openLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){return select([&](auto&t){return t.closeLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedLiveTransport::statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&r){return select([&](auto&t){return t.statusLiveSource(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::discoverNativeProbe(){return select([](auto&t){return t.discoverNativeProbe();});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::executeNativeProbe(const SuiteBridgeNativeProbeRequest&r){return select([&](auto&t){return t.executeNativeProbe(r);});}
SuiteBridgeCommandReply SuiteBridgePrioritizedNativeProbeTransport::readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&r){return select([&](auto&t){return t.readNativeProbe(r);});}
} // namespace vdrsuite::agent
