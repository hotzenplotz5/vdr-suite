#include "SuiteBridgeNativeTimerModifyTransport.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace {
constexpr const char* CapabilityProtocol="vdr-suite-ntmod-cap/1";
constexpr const char* ResultProtocol="vdr-suite-ntmod-result/1";
constexpr int CapabilityReplyCode=900;
constexpr int AcceptedCode=557;
constexpr int UnknownCode=558;
bool safeToken(const std::string& value,std::size_t max=512){
 return !value.empty()&&value.size()<=max&&std::all_of(value.begin(),value.end(),
 [](unsigned char c){return std::isalnum(c)||c=='-'||c=='_'||c=='.'||c==':';});}
std::vector<std::string> split(const std::string& in,std::size_t max){
 std::vector<std::string> out;std::size_t pos=0;while(pos<in.size()){
  while(pos<in.size()&&in[pos]==' '){++pos;}
  if(pos==in.size())break;
  auto end=in.find(' ',pos);out.push_back(in.substr(pos,end==std::string::npos?end:end-pos));
  if(out.size()>max){return {};}
  if(end==std::string::npos)break;
  pos=end+1;}
 return out;}
bool number(const std::string& s,std::uint64_t& n){if(s.empty())return false;n=0;for(char c:s){
 if(c<'0'||c>'9'){return false;}
 auto d=static_cast<std::uint64_t>(c-'0');
 if(n>(std::numeric_limits<std::uint64_t>::max()-d)/10){return false;}
 n=n*10+d;}
 return n>0;}
char hd(unsigned n){return n<10?static_cast<char>('0'+n):static_cast<char>('a'+n-10);}
std::string hexText(const std::string& value){if(value.empty())return "-";std::string out;
 out.reserve(value.size()*2);for(unsigned char c:value){out.push_back(hd(c>>4));out.push_back(hd(c&15));}return out;}
BackendAgentNativeTimerModifyTransportReply result(
 BackendAgentNativeTimerModifyTransportDisposition d,std::string e){return {d,std::move(e)};}
bool parseCapability(const SuiteBridgeCommandReply& reply,const char* capability,
 std::string& epoch,std::uint64_t& generation,std::uint64_t& revision,
 bool& enabled,std::string& reason){
 if(!reply.transportSucceeded()||reply.replyCode!=CapabilityReplyCode){reason="ntmod_capability_unavailable";return false;}
 auto v=split(reply.payload,10);if(v.size()!=10||v[0]!=CapabilityProtocol||v[1]!=capability||
 v[2]!="1"||v[3]!="timer-modify"||(v[4]!="enabled"&&v[4]!="disabled")||v[9]!=v[4]||
 v[5]!=kBackendAgentNativeTimerModifyProviderKind||!safeToken(v[6],192)||
 !number(v[7],generation)||!number(v[8],revision)){reason="ntmod_capability_invalid";return false;}
 epoch=v[6];enabled=v[4]=="enabled";return true;}
}

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::discoverNativeTimerModifyContract(
    BackendAgentNativeTimerModifyKind kind)
{
 return executeRequest(std::string("PLUG suitebridge NTMOD CAP 1 ")+
   backendAgentNativeTimerModifyKindName(kind)+"\r\n");
}

SuiteBridgeCommandReply SuiteBridgeSvdrpTransport::executeNativeTimerModifyContract(
    const BackendAgentNativeTimerModifyTransportRequest& request)
{
 const auto& c=request.command;const auto& q=c.localProviderSelection;const auto& s=c.specification;
 std::ostringstream wire;wire<<"PLUG suitebridge NTMOD EXEC vdr-suite-native/1 "
 <<backendAgentNativeTimerModifyCapability(c.kind)<<" 1 "<<c.commandId<<' '
 <<c.requestFingerprint<<' '<<c.operationId<<' '<<c.operationRevision<<' '
 <<c.nativeTimerBindingId<<' '<<c.expectedBindingRevision<<' '
 <<c.expectedCurrentFingerprint<<' '<<c.timerAssignmentId<<' '<<c.backendNativeTimerId<<' '
 <<hexText(s.channelId)<<' '<<hexText(s.title)<<' '<<hexText(s.directory)<<' '
 <<hexText(s.day)<<' '<<hexText(s.weekdays)<<' '<<hexText(s.startTime)<<' '
 <<hexText(s.endTime)<<' '<<s.priority<<' '<<s.lifetime<<' '
 <<(s.enabled?1:0)<<' '<<(s.vps?1:0)<<' '<<c.jobId<<' '<<c.attemptId<<' '
 <<c.claimEpoch<<' '<<c.backendId<<' '<<c.agentId<<' '<<c.agentInstanceId<<' '
 <<c.backendGeneration<<' '<<c.controlPlaneClaimedAt<<' '<<q.authorityDomain<<' '
 <<q.providerId<<' '<<q.providerKind<<' '<<q.ownershipGeneration<<' '
 <<q.providerInstanceEpoch<<' '<<q.providerGeneration<<' '<<q.capabilityRevision<<' '
 <<q.requiredCapability<<' '<<request.localStartingPersistedAt<<"\r\n";
 return executeRequest(wire.str());
}

SuiteBridgeNativeTimerModifyTransport::SuiteBridgeNativeTimerModifyTransport(
 SuiteBridgeSvdrpTransportConfig config):transport_(std::move(config)){}

bool SuiteBridgeNativeTimerModifyTransport::discoverProvider(
 BackendAgentLocalProviderFacts& facts,std::string& reason)
{
 facts={};std::string updateEpoch,toggleEpoch;std::uint64_t ug=0,ur=0,tg=0,tr=0;
 bool updateEnabled=false,toggleEnabled=false;
 if(!parseCapability(transport_.discoverNativeTimerModifyContract(
      BackendAgentNativeTimerModifyKind::update),kBackendAgentNativeTimerUpdateCapability,
      updateEpoch,ug,ur,updateEnabled,reason)||
    !parseCapability(transport_.discoverNativeTimerModifyContract(
      BackendAgentNativeTimerModifyKind::toggle),kBackendAgentNativeTimerToggleCapability,
      toggleEpoch,tg,tr,toggleEnabled,reason)||updateEpoch!=toggleEpoch||ug!=tg||ur!=tr||
    updateEnabled!=toggleEnabled){
   reason="native_timer_modify_suitebridge_capability_invalid";return false;}
 facts.providerId=kBackendAgentNativeTimerModifyProviderId;
 facts.providerKind=kBackendAgentNativeTimerModifyProviderKind;
 facts.providerInstanceEpoch=updateEpoch;facts.providerGeneration=ug;
 facts.capabilityRevision=ur;facts.available=updateEnabled;
 facts.capabilities={kBackendAgentNativeTimerUpdateCapability,
                     kBackendAgentNativeTimerToggleCapability};
 if(!backendAgentLocalProviderValidFacts(facts)){facts={};reason="native_timer_modify_provider_invalid";return false;}
 reason=updateEnabled?"native_timer_modify_suitebridge_provider_discovered":
                      "native_timer_modify_suitebridge_provider_discovered_disabled";
 return true;
}

BackendAgentNativeTimerModifyTransportReply
SuiteBridgeNativeTimerModifyTransport::modifyTimer(
 const BackendAgentNativeTimerModifyTransportRequest& request)
{
 std::string validation;
 if(!backendAgentNativeTimerModifyValidCommand(request.command,validation)||
    request.localStartingPersistedAt<request.command.controlPlaneClaimedAt)
   return result(BackendAgentNativeTimerModifyTransportDisposition::rejectedWithoutEffect,
                 "suitebridge:ntmod:local-request-invalid");
 const auto reply=transport_.executeNativeTimerModifyContract(request);
 if(!reply.transportSucceeded())
   return result(BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown,
                 "suitebridge:ntmod:transport-outcome-unknown");
 const auto& c=request.command;const auto& q=c.localProviderSelection;
 auto v=split(reply.payload,11);std::uint64_t generation=0,revision=0;
 if(v.size()!=11||v[0]!=ResultProtocol||v[1]!=c.commandId||
    v[2]!=c.requestFingerprint||v[3]!=backendAgentNativeTimerModifyCapability(c.kind)||
    v[4]!="1"||!safeToken(v[5],192)||!number(v[6],generation)||
    !number(v[7],revision)||!safeToken(v[8],64)||!safeToken(v[9],64)||!safeToken(v[10]))
   return result(BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown,
                 "suitebridge:ntmod:reply-outcome-unknown");
 if(reply.replyCode!=555&&(v[5]!=q.providerInstanceEpoch||
    generation!=q.providerGeneration||revision!=q.capabilityRevision))
   return result(BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown,
                 "suitebridge:ntmod:reply-fence-mismatch");
 if(reply.replyCode==AcceptedCode&&v[8]=="accepted_unverified")
   return result(BackendAgentNativeTimerModifyTransportDisposition::acceptedUnverified,v[10]);
 if(reply.replyCode==UnknownCode&&v[8]=="outcome_unknown")
   return result(BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown,v[10]);
 if((reply.replyCode==555||reply.replyCode==556||reply.replyCode==559||
     reply.replyCode==560)&&v[8]=="rejected_without_effect")
   return result(BackendAgentNativeTimerModifyTransportDisposition::rejectedWithoutEffect,v[10]);
 return result(BackendAgentNativeTimerModifyTransportDisposition::outcomeUnknown,
               "suitebridge:ntmod:reply-outcome-unknown");
}
}
