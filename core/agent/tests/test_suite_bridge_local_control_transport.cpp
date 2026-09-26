#include "SuiteBridgeLocalControlTransport.h"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <mutex>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace vdrsuite::agent;using namespace vdrsuite::agent::control;
namespace {
class SlowCompatibilityTransport final:public ISuiteBridgeLiveSourceTransport {
public: std::timed_mutex lane;std::atomic<int> statusCalls{0};
 SuiteBridgeCommandReply discoverLiveSource()override{return{SuiteBridgeTransportStatus::Success,250,"{}",{}};}
 SuiteBridgeCommandReply openLiveSource(const SuiteBridgeLiveSourceOpenRequest&)override{return{SuiteBridgeTransportStatus::Success,250,"{}",{}};}
 SuiteBridgeCommandReply closeLiveSource(const SuiteBridgeLiveSourceLeaseRequest&)override{return{SuiteBridgeTransportStatus::Success,250,"{}",{}};}
 SuiteBridgeCommandReply statusLiveSource(const SuiteBridgeLiveSourceLeaseRequest&)override{++statusCalls;std::lock_guard<std::timed_mutex>g(lane);return{SuiteBridgeTransportStatus::Success,250,"{}",{}};}
};
class NativeProbeCompatibility final:public IBackendAgentNativeProbeTransport {
public: std::atomic<int> executeCalls{0};
 SuiteBridgeCommandReply discoverNativeProbe()override{return{SuiteBridgeTransportStatus::Success,900,"{}",{}};}
 SuiteBridgeCommandReply executeNativeProbe(const SuiteBridgeNativeProbeRequest&)override{++executeCalls;return{SuiteBridgeTransportStatus::Success,900,"{}",{}};}
 SuiteBridgeCommandReply readNativeProbe(const SuiteBridgeNativeProbeReadbackRequest&)override{return{SuiteBridgeTransportStatus::Success,900,"{}",{}};}
};
void serveOne(const std::string&path){int listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);assert(listener>=0);sockaddr_un a{};a.sun_family=AF_UNIX;std::copy(path.begin(),path.end(),a.sun_path);a.sun_path[path.size()]='\0';unlink(path.c_str());assert(bind(listener,reinterpret_cast<const sockaddr*>(&a),sizeof(a))==0);assert(listen(listener,4)==0);int client=accept(listener,nullptr,nullptr);assert(client>=0);std::vector<std::uint8_t>b(MaximumRequestFrameBytes+1);ssize_t n=recv(client,b.data(),b.size(),MSG_TRUNC);assert(n>0);b.resize(static_cast<std::size_t>(n));Request q;std::string why;assert(decodeRequest(b,q,why));assert(q.operation==Operation::LiveStatus);Response r;r.result=Result::Success;r.requestId=q.requestId;r.replyCode=250;r.payload="{\"state\":\"streaming\"}";assert(encodeResponse(r,b));assert(send(client,b.data(),b.size(),0)==static_cast<ssize_t>(b.size()));close(client);close(listener);unlink(path.c_str());}
void serveNativeProbe(const std::string&path){int listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);assert(listener>=0);sockaddr_un a{};a.sun_family=AF_UNIX;std::copy(path.begin(),path.end(),a.sun_path);a.sun_path[path.size()]='\0';unlink(path.c_str());assert(bind(listener,reinterpret_cast<const sockaddr*>(&a),sizeof(a))==0);assert(listen(listener,4)==0);int client=accept(listener,nullptr,nullptr);assert(client>=0);std::vector<std::uint8_t>b(MaximumRequestFrameBytes+1);ssize_t n=recv(client,b.data(),b.size(),MSG_TRUNC);assert(n>0);b.resize(static_cast<std::size_t>(n));Request q;std::string why;assert(decodeRequest(b,q,why));assert(q.operation==Operation::NativeProbeExecute);assert(std::count(q.payload.begin(),q.payload.end(),'\n')==11);Response r;r.result=Result::Success;r.requestId=q.requestId;r.replyCode=900;r.payload="{\"nativeOperation\":\"vdr.native.probe\"}";assert(encodeResponse(r,b));assert(send(client,b.data(),b.size(),0)==static_cast<ssize_t>(b.size()));close(client);close(listener);unlink(path.c_str());}
}
int main(){
 std::string path="/tmp/vdr-suite-local-control-client-"+std::to_string(getpid())+".sock";std::thread server([&]{serveOne(path);});while(access(path.c_str(),F_OK)!=0)std::this_thread::sleep_for(std::chrono::milliseconds(1));
 SlowCompatibilityTransport compatibility;SuiteBridgeLocalControlTransportConfig config;config.socketPath=path;config.connectTimeout=std::chrono::milliseconds(100);config.ioTimeout=std::chrono::milliseconds(200);config.operationTimeout=std::chrono::milliseconds(300);SuiteBridgeLocalControlTransport dedicated(config);SuiteBridgePrioritizedLiveTransport routed(dedicated,compatibility);
 compatibility.lane.lock();SuiteBridgeLiveSourceLeaseRequest request{"lease-1","epoch-1"};auto started=std::chrono::steady_clock::now();auto reply=routed.statusLiveSource(request);auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-started);compatibility.lane.unlock();
 assert(reply.transportSucceeded());assert(reply.replyCode==250);assert(elapsed<std::chrono::milliseconds(150));assert(compatibility.statusCalls.load()==0);server.join();
 SuiteBridgeLocalControlTransportConfig missingConfig;missingConfig.socketPath=path+".missing";SuiteBridgeLocalControlTransport missing(missingConfig);SuiteBridgePrioritizedLiveTransport fallback(missing,compatibility);auto fallbackReply=fallback.statusLiveSource(request);assert(fallbackReply.transportSucceeded());assert(compatibility.statusCalls.load()==1);
 std::string nativePath="/tmp/vdr-suite-native-probe-control-"+std::to_string(getpid())+".sock";std::thread nativeServer([&]{serveNativeProbe(nativePath);});while(access(nativePath.c_str(),F_OK)!=0)std::this_thread::sleep_for(std::chrono::milliseconds(1));
 SuiteBridgeLocalControlTransportConfig nativeConfig;nativeConfig.socketPath=nativePath;SuiteBridgeLocalControlTransport nativeDedicated(nativeConfig);NativeProbeCompatibility nativeCompatibility;SuiteBridgePrioritizedNativeProbeTransport nativeRouted(nativeDedicated,nativeCompatibility);
 SuiteBridgeNativeProbeRequest probe;probe.commandId="command-1";probe.requestFingerprint="fingerprint-1";probe.operationId="operation-1";probe.jobId="job-1";probe.attemptId="attempt-1";probe.claimEpoch=1;probe.backendId="backend-1";probe.agentId="agent-1";probe.agentInstanceId="instance-1";probe.backendGeneration=1;probe.pluginInstanceEpoch="epoch-1";probe.probeNonce="nonce-1";
 auto probeReply=nativeRouted.executeNativeProbe(probe);assert(probeReply.transportSucceeded());assert(probeReply.replyCode==900);assert(nativeCompatibility.executeCalls.load()==0);nativeServer.join();
 SuiteBridgeLocalControlTransportConfig nativeMissingConfig;nativeMissingConfig.socketPath=nativePath+".missing";SuiteBridgeLocalControlTransport nativeMissing(nativeMissingConfig);SuiteBridgePrioritizedNativeProbeTransport nativeFallback(nativeMissing,nativeCompatibility);auto probeFallback=nativeFallback.executeNativeProbe(probe);assert(probeFallback.transportSucceeded());assert(nativeCompatibility.executeCalls.load()==1);
 std::puts("SuiteBridge local control transport HOL/native-probe fallback tests passed");
}
