#include "BackendAgentNativeTimerModifyPayload.h"

#include "BackendAgentCommand.h"

#include <cstddef>
#include <limits>
#include <utility>
#include <vector>

namespace vdrsuite::agent
{
namespace {
constexpr const char* Prefix="native-timer-modify-agent-payload/1|";
constexpr std::size_t Count=34;
void append(std::string& out,const std::string& value) {
    out+=std::to_string(value.size())+":"+value+"|";
}
void append(std::string& out,std::uint64_t value) { append(out,std::to_string(value)); }
void append(std::string& out,std::int32_t value) { append(out,std::to_string(value)); }
void appendBoolean(std::string& out,bool value) {
    append(out,std::string(value ? "1" : "0"));
}
bool read(const std::string& in,std::size_t& pos,std::string& value) {
    std::size_t length=0; bool seen=false;
    while(pos<in.size()&&in[pos]!=':') {
        const char c=in[pos++]; if(c<'0'||c>'9') return false; seen=true;
        const std::size_t d=static_cast<std::size_t>(c-'0');
        if(length>(std::numeric_limits<std::size_t>::max()-d)/10) return false;
        length=length*10+d;
    }
    if(!seen||pos>=in.size()||in[pos++]!=':'||length>in.size()-pos) return false;
    value=in.substr(pos,length); pos+=length;
    if(pos>=in.size()||in[pos++]!='|') return false;
    return true;
}
bool number(const std::string& token,std::uint64_t& value,bool positive=true) {
    if(token.empty()) return false;
    value=0;
    for(char c:token) { if(c<'0'||c>'9') return false;
        const auto d=static_cast<std::uint64_t>(c-'0');
        if(value>(std::numeric_limits<std::uint64_t>::max()-d)/10) return false;
        value=value*10+d;
    }
    return !positive||value>0;
}
bool small(const std::string& token,std::int32_t& value) {
    std::uint64_t n=0; if(!number(token,n,false)||n>99) return false;
    value=static_cast<std::int32_t>(n); return true;
}
bool boolean(const std::string& token,bool& value) {
    if(token=="0"){value=false;return true;} if(token=="1"){value=true;return true;}
    return false;
}
std::string hhmm(const std::string& value) {
    return std::string(4-value.size(),'0')+value;
}
}

bool backendAgentNativeTimerModifyPayloadValid(
    const BackendAgentNativeTimerModifyPayload& p)
{
    return backendAgentCommandSafeIdentifier(p.operationRevision) &&
        backendAgentCommandSafeIdentifier(p.timerAssignmentId) &&
        backendAgentCommandSafeIdentifier(p.expectedAssignmentRevision) &&
        backendAgentCommandSafeIdentifier(p.expectedIntentRevision) &&
        p.assignmentEpoch>0 && backendAgentCommandSafeIdentifier(p.nativeTimerBindingId) &&
        backendAgentCommandSafeIdentifier(p.expectedBindingRevision) &&
        backendAgentCommandSafeIdentifier(p.backendId) && p.backendGeneration>0 &&
        backendAgentCommandSafeIdentifier(p.backendNativeTimerId) &&
        !p.expectedCurrentFingerprint.empty() &&
        p.expectedSpecificationFingerprint==
            backendAgentNativeTimerCreateSpecificationFingerprint(p.specification) &&
        p.controlPlaneClaimedAt>0 &&
        backendAgentLocalProviderValidSelection(p.localProviderSelection) &&
        p.localProviderSelection.backendId==p.backendId &&
        p.localProviderSelection.authorityDomain==kBackendAgentNativeTimerModifyAuthorityDomain &&
        p.localProviderSelection.providerId==kBackendAgentNativeTimerModifyProviderId &&
        p.localProviderSelection.providerKind==kBackendAgentNativeTimerModifyProviderKind &&
        p.localProviderSelection.requiredCapability==
            backendAgentNativeTimerModifyCapability(p.kind);
}

std::string backendAgentNativeTimerModifyPayload(
    const BackendAgentNativeTimerModifyPayload& p)
{
    if(!backendAgentNativeTimerModifyPayloadValid(p)) return {};
    std::string out(Prefix);
    append(out,backendAgentNativeTimerModifyKindName(p.kind));
    append(out,p.operationRevision); append(out,p.timerAssignmentId);
    append(out,p.expectedAssignmentRevision); append(out,p.expectedIntentRevision);
    append(out,p.assignmentEpoch); append(out,p.nativeTimerBindingId);
    append(out,p.expectedBindingRevision); append(out,p.backendId);
    append(out,p.backendGeneration); append(out,p.backendNativeTimerId);
    append(out,p.expectedCurrentFingerprint); append(out,p.expectedSpecificationFingerprint);
    append(out,static_cast<std::uint64_t>(p.controlPlaneClaimedAt));
    const auto& s=p.specification;
    append(out,s.channelId); append(out,s.title); append(out,s.directory); append(out,s.day);
    append(out,s.weekdays); append(out,hhmm(s.startTime)); append(out,hhmm(s.endTime));
    append(out,s.priority); append(out,s.lifetime);
    appendBoolean(out,s.enabled); appendBoolean(out,s.vps);
    const auto& q=p.localProviderSelection;
    append(out,q.backendId); append(out,q.authorityDomain); append(out,q.providerId);
    append(out,q.providerKind); append(out,q.ownershipGeneration);
    append(out,q.providerInstanceEpoch); append(out,q.providerGeneration);
    append(out,q.capabilityRevision); append(out,q.requiredCapability);
    return out;
}

bool backendAgentNativeTimerModifyParsePayload(
    const std::string& encoded, BackendAgentNativeTimerModifyPayload& payload,
    std::string& reason)
{
    const std::string prefix(Prefix);
    if(encoded.compare(0,prefix.size(),prefix)!=0) {
        reason="invalid_native_timer_modify_payload"; return false;
    }
    std::size_t pos=prefix.size(); std::vector<std::string> f;
    while(pos<encoded.size()) {
        std::string value;
        if(!read(encoded,pos,value)||f.size()>=Count) {
            reason="invalid_native_timer_modify_payload"; return false;
        }
        f.push_back(std::move(value));
    }
    if(f.size()!=Count) { reason="invalid_native_timer_modify_payload"; return false; }
    BackendAgentNativeTimerModifyPayload p;
    if(f[0]=="update") p.kind=BackendAgentNativeTimerModifyKind::update;
    else if(f[0]=="toggle") p.kind=BackendAgentNativeTimerModifyKind::toggle;
    else { reason="invalid_native_timer_modify_payload"; return false; }
    p.operationRevision=f[1]; p.timerAssignmentId=f[2];
    p.expectedAssignmentRevision=f[3]; p.expectedIntentRevision=f[4];
    p.nativeTimerBindingId=f[6]; p.expectedBindingRevision=f[7]; p.backendId=f[8];
    p.backendNativeTimerId=f[10]; p.expectedCurrentFingerprint=f[11];
    p.expectedSpecificationFingerprint=f[12];
    std::uint64_t claimed=0;
    if(!number(f[5],p.assignmentEpoch)||!number(f[9],p.backendGeneration)||
       !number(f[13],claimed)||
       claimed>static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
        reason="invalid_native_timer_modify_payload"; return false;
    }
    p.controlPlaneClaimedAt=static_cast<std::int64_t>(claimed);
    auto& s=p.specification;
    s.channelId=f[14];s.title=f[15];s.directory=f[16];s.day=f[17];
    s.weekdays=f[18];s.startTime=f[19];s.endTime=f[20];
    if(!small(f[21],s.priority)||!small(f[22],s.lifetime)||
       !boolean(f[23],s.enabled)||!boolean(f[24],s.vps)) {
        reason="invalid_native_timer_modify_payload"; return false;
    }
    auto& q=p.localProviderSelection;
    q.backendId=f[25];q.authorityDomain=f[26];q.providerId=f[27];q.providerKind=f[28];
    q.providerInstanceEpoch=f[30];q.requiredCapability=f[33];
    if(!number(f[29],q.ownershipGeneration)||!number(f[31],q.providerGeneration)||
       !number(f[32],q.capabilityRevision)||
       !backendAgentNativeTimerModifyPayloadValid(p)||
       backendAgentNativeTimerModifyPayload(p)!=encoded) {
        reason="invalid_native_timer_modify_payload"; return false;
    }
    payload=std::move(p); reason.clear(); return true;
}
}
