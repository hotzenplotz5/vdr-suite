#include "BackendAgentNativeTimerModifyLocalState.h"

#include "BackendAgentNativeTimerModifyPayload.h"

#include <algorithm>
#include <limits>
#include <map>
#include <sstream>
#include <vector>

namespace vdrsuite::agent
{
namespace {
constexpr std::size_t MaxBytes=40U*1024U;
char hexDigit(unsigned n){return n<10?static_cast<char>('0'+n):static_cast<char>('a'+n-10);}
std::string hex(const std::string& value){std::string out;out.reserve(value.size()*2);
 for(unsigned char c:value){out.push_back(hexDigit(c>>4));out.push_back(hexDigit(c&15));}return out;}
int hexValue(char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;return -1;}
bool unhex(const std::string& in,std::string& out){out.clear();if(in.size()%2)return false;
 for(std::size_t i=0;i<in.size();i+=2){int h=hexValue(in[i]),l=hexValue(in[i+1]);
  if(h<0||l<0)return false;out.push_back(static_cast<char>((h<<4)|l));}return true;}
bool number(const std::string& s,std::uint64_t& n){if(s.empty())return false;n=0;
 for(char c:s){if(c<'0'||c>'9')return false;auto d=static_cast<std::uint64_t>(c-'0');
 if(n>(static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())-d)/10)return false;n=n*10+d;}return true;}
const std::vector<std::string>& keys(){static const std::vector<std::string> k={
 "schema","phase","command_id","request_fingerprint","operation_id","payload_hex",
 "job_id","attempt_id","claim_epoch","agent_id","agent_instance_id",
 "local_starting_persisted_at","outcome","dispatch_started_at","completed_at",
 "evidence_reference"};return k;}
bool exact(const std::map<std::string,std::string>& v){if(v.size()!=keys().size())return false;
 for(const auto& k:keys())if(v.count(k)!=1)return false;return true;}
const char* outcome(BackendAgentNativeTimerModifyOutcomeCategory o){switch(o){
 case BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect:return "rejected_without_effect";
 case BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified:return "accepted_unverified";
 case BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown:return "outcome_unknown";}return "invalid";}
bool parseOutcome(const std::string& s,BackendAgentNativeTimerModifyOutcomeCategory& o){
 if(s=="rejected_without_effect")o=BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect;
 else if(s=="accepted_unverified")o=BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified;
 else if(s=="outcome_unknown")o=BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown;
 else return false;return true;}
bool emptyEvidence(const BackendAgentNativeTimerModifyEvidence& e){
 return e.commandId.empty()&&e.evidenceReference.empty()&&e.completedAt==0&&
   e.localStartingPersistedAt==0&&e.dispatchStartedAt==0;}
BackendAgentNativeTimerModifyPayload payloadFor(const BackendAgentNativeTimerModifyCommand& c){
 BackendAgentNativeTimerModifyPayload p; p.kind=c.kind;p.operationRevision=c.operationRevision;
 p.timerAssignmentId=c.timerAssignmentId;p.expectedAssignmentRevision=c.expectedAssignmentRevision;
 p.expectedIntentRevision=c.expectedIntentRevision;p.assignmentEpoch=c.assignmentEpoch;
 p.nativeTimerBindingId=c.nativeTimerBindingId;p.expectedBindingRevision=c.expectedBindingRevision;
 p.backendId=c.backendId;p.backendGeneration=c.backendGeneration;
 p.backendNativeTimerId=c.backendNativeTimerId;p.expectedCurrentFingerprint=c.expectedCurrentFingerprint;
 p.expectedSpecificationFingerprint=c.expectedSpecificationFingerprint;p.specification=c.specification;
 p.controlPlaneClaimedAt=c.controlPlaneClaimedAt;p.localProviderSelection=c.localProviderSelection;return p;}
BackendAgentNativeTimerModifyCommand commandFor(
 const std::string& commandId,const std::string& fingerprint,const std::string& operationId,
 const std::string& job,const std::string& attempt,std::uint64_t claim,
 const std::string& agent,const std::string& instance,
 const BackendAgentNativeTimerModifyPayload& p){
 BackendAgentNativeTimerModifyCommand c;c.kind=p.kind;c.commandId=commandId;
 c.requestFingerprint=fingerprint;c.operationId=operationId;c.operationRevision=p.operationRevision;
 c.timerAssignmentId=p.timerAssignmentId;c.expectedAssignmentRevision=p.expectedAssignmentRevision;
 c.expectedIntentRevision=p.expectedIntentRevision;c.assignmentEpoch=p.assignmentEpoch;
 c.nativeTimerBindingId=p.nativeTimerBindingId;c.expectedBindingRevision=p.expectedBindingRevision;
 c.backendNativeTimerId=p.backendNativeTimerId;c.expectedCurrentFingerprint=p.expectedCurrentFingerprint;
 c.expectedSpecificationFingerprint=p.expectedSpecificationFingerprint;c.specification=p.specification;
 c.jobId=job;c.attemptId=attempt;c.claimEpoch=claim;c.backendId=p.backendId;c.agentId=agent;
 c.agentInstanceId=instance;c.backendGeneration=p.backendGeneration;
 c.controlPlaneClaimedAt=p.controlPlaneClaimedAt;c.localProviderSelection=p.localProviderSelection;return c;}
BackendAgentNativeTimerModifyEvidence evidenceFor(const BackendAgentNativeTimerModifyCommand& c,
 std::int64_t start,BackendAgentNativeTimerModifyOutcomeCategory o,std::int64_t dispatch,
 std::int64_t completed,const std::string& ref){
 BackendAgentNativeTimerModifyEvidence e;e.commandId=c.commandId;e.requestFingerprint=c.requestFingerprint;
 e.operationId=c.operationId;e.operationRevision=c.operationRevision;e.jobId=c.jobId;e.attemptId=c.attemptId;
 e.claimEpoch=c.claimEpoch;e.backendId=c.backendId;e.agentId=c.agentId;e.agentInstanceId=c.agentInstanceId;
 e.backendGeneration=c.backendGeneration;e.providerInstanceEpoch=c.localProviderSelection.providerInstanceEpoch;
 e.localStartingPersistedAt=start;e.outcome=o;e.dispatchStartedAt=dispatch;e.completedAt=completed;
 e.evidenceReference=ref;return e;}
}

bool backendAgentNativeTimerModifyCommandFromAssignment(
 const BackendAgentCommandAssignment& a,BackendAgentNativeTimerModifyCommand& command,std::string& reason){
 if(!::backendAgentCommandValidAssignment(a)||
    (a.commandType!=kBackendAgentNativeTimerUpdateCommandType&&
     a.commandType!=kBackendAgentNativeTimerToggleCommandType)||
    a.payloadVersion!=kBackendAgentNativeTimerModifyPayloadVersion||
    a.verificationPolicy!="readback_required"){reason="invalid_native_timer_modify_assignment";return false;}
 BackendAgentNativeTimerModifyPayload p;
 if(!backendAgentNativeTimerModifyParsePayload(a.payload,p,reason)||
    (a.commandType==kBackendAgentNativeTimerUpdateCommandType)!=(p.kind==BackendAgentNativeTimerModifyKind::update)||
    p.backendId!=a.backendId||p.backendGeneration!=a.backendGeneration){
    reason="invalid_native_timer_modify_assignment_payload";return false;}
 auto c=commandFor(a.commandId,a.requestFingerprint,a.operationId,a.jobId,a.attemptId,
   a.claimEpoch,a.agentId,a.agentInstanceId,p);
 if(!backendAgentNativeTimerModifyValidCommand(c,reason)){
   reason="invalid_native_timer_modify_command_envelope";return false;}
 command=std::move(c);reason.clear();return true;
}

bool backendAgentNativeTimerModifyPrepareLocalStarting(
 const BackendAgentCommandAssignment& a,std::int64_t now,
 BackendAgentNativeTimerModifyLocalState& state,std::string& reason){
 BackendAgentNativeTimerModifyCommand c;
 if(!backendAgentNativeTimerModifyCommandFromAssignment(a,c,reason))return false;
 if(now<c.controlPlaneClaimedAt||now>a.deadline){reason="native_timer_modify_starting_time_fenced";return false;}
 BackendAgentNativeTimerModifyLocalState s;s.command=std::move(c);s.localStartingPersistedAt=now;
 if(!backendAgentNativeTimerModifyLocalStateValid(s,reason))return false;
 state=std::move(s);reason="native_timer_modify_starting_ready_for_durable_persist";return true;
}

bool backendAgentNativeTimerModifyLocalStateValid(
 const BackendAgentNativeTimerModifyLocalState& s,std::string& reason){
 if(s.schemaVersion!=1||!backendAgentNativeTimerModifyValidCommand(s.command,reason)||
    s.localStartingPersistedAt<s.command.controlPlaneClaimedAt){
   reason="invalid_native_timer_modify_local_state";return false;}
 if(s.phase==BackendAgentNativeTimerModifyLocalPhase::starting){
   if(!emptyEvidence(s.evidence)){reason="starting_native_timer_modify_has_evidence";return false;}
   reason.clear();return true;}
 if(s.phase!=BackendAgentNativeTimerModifyLocalPhase::completed||
    !backendAgentNativeTimerModifyEvidenceMatches(s.evidence,s.command,reason)||
    s.evidence.localStartingPersistedAt!=s.localStartingPersistedAt){
   reason="invalid_native_timer_modify_completed_state";return false;}
 reason.clear();return true;
}

bool backendAgentNativeTimerModifyCompleteLocalState(
 BackendAgentNativeTimerModifyLocalState& s,const BackendAgentNativeTimerModifyEvidence& e,
 std::string& reason){
 if(!backendAgentNativeTimerModifyLocalStateValid(s,reason)||
    s.phase!=BackendAgentNativeTimerModifyLocalPhase::starting||
    e.localStartingPersistedAt!=s.localStartingPersistedAt||
    !backendAgentNativeTimerModifyEvidenceMatches(e,s.command,reason)){
   reason="native_timer_modify_completion_evidence_mismatch";return false;}
 auto candidate=s;candidate.phase=BackendAgentNativeTimerModifyLocalPhase::completed;
 candidate.evidence=e;if(!backendAgentNativeTimerModifyLocalStateValid(candidate,reason))return false;
 s=std::move(candidate);reason="native_timer_modify_completion_ready_for_durable_persist";return true;
}

BackendAgentNativeTimerModifyRecoveryResult backendAgentNativeTimerModifyRecoverLocalState(
 const BackendAgentNativeTimerModifyLocalState& s,const std::string& backend,
 const std::string& agent,const std::string& instance,std::uint64_t generation,std::int64_t now){
 BackendAgentNativeTimerModifyRecoveryResult r;std::string reason;
 if(!backendAgentNativeTimerModifyLocalStateValid(s,reason)||backend.empty()||agent.empty()||
    instance.empty()||generation==0||now<=0){r.reasonCode="native_timer_modify_recovery_invalid";return r;}
 if(s.phase==BackendAgentNativeTimerModifyLocalPhase::completed){
   r.decision=BackendAgentNativeTimerModifyRecoveryDecision::returnPersistedEvidence;
   r.evidence=s.evidence;r.reasonCode="native_timer_modify_completed_evidence_replay";return r;}
 r.evidence=evidenceFor(s.command,s.localStartingPersistedAt,
   BackendAgentNativeTimerModifyOutcomeCategory::outcomeUnknown,s.localStartingPersistedAt,
   std::max(now,s.localStartingPersistedAt),"local-recovery:"+s.command.commandId);
 if(!backendAgentNativeTimerModifyEvidenceMatches(r.evidence,s.command,reason)){
   r.reasonCode="native_timer_modify_recovery_evidence_invalid";return r;}
 r.decision=BackendAgentNativeTimerModifyRecoveryDecision::reconcileOnly;
 r.reasonCode=(backend==s.command.backendId&&agent==s.command.agentId&&
   instance==s.command.agentInstanceId&&generation==s.command.backendGeneration)
   ?"native_timer_modify_starting_recovery_reconcile_only"
   :"native_timer_modify_starting_context_fenced_reconcile_only";
 return r;
}

std::string backendAgentNativeTimerModifySerializeLocalState(
 const BackendAgentNativeTimerModifyLocalState& s,std::string& reason){
 if(!backendAgentNativeTimerModifyLocalStateValid(s,reason))return {};
 const bool complete=s.phase==BackendAgentNativeTimerModifyLocalPhase::completed;
 const auto payload=backendAgentNativeTimerModifyPayload(payloadFor(s.command));
 std::ostringstream out;out<<"schema=1\nphase="<<(complete?"completed":"starting")
 <<"\ncommand_id="<<s.command.commandId<<"\nrequest_fingerprint="<<s.command.requestFingerprint
 <<"\noperation_id="<<s.command.operationId<<"\npayload_hex="<<hex(payload)
 <<"\njob_id="<<s.command.jobId<<"\nattempt_id="<<s.command.attemptId
 <<"\nclaim_epoch="<<s.command.claimEpoch<<"\nagent_id="<<s.command.agentId
 <<"\nagent_instance_id="<<s.command.agentInstanceId
 <<"\nlocal_starting_persisted_at="<<s.localStartingPersistedAt
 <<"\noutcome="<<(complete?outcome(s.evidence.outcome):"none")
 <<"\ndispatch_started_at="<<(complete?s.evidence.dispatchStartedAt:0)
 <<"\ncompleted_at="<<(complete?s.evidence.completedAt:0)
 <<"\nevidence_reference="<<(complete?s.evidence.evidenceReference:"")<<"\n";
 const auto encoded=out.str();if(encoded.size()>MaxBytes){reason="native_timer_modify_state_too_large";return {};}
 reason.clear();return encoded;
}

bool backendAgentNativeTimerModifyParseLocalState(
 const std::string& encoded,BackendAgentNativeTimerModifyLocalState& state,std::string& reason){
 if(encoded.empty()||encoded.size()>MaxBytes){reason="invalid_native_timer_modify_state";return false;}
 std::map<std::string,std::string> v;std::istringstream in(encoded);std::string line;
 while(std::getline(in,line)){auto p=line.find('=');if(p==std::string::npos||p==0||
   !v.emplace(line.substr(0,p),line.substr(p+1)).second){reason="invalid_native_timer_modify_state";return false;}}
 if(!exact(v)||v["schema"]!="1"){reason="invalid_native_timer_modify_state_schema";return false;}
 BackendAgentNativeTimerModifyLocalState s;
 if(v["phase"]=="completed")s.phase=BackendAgentNativeTimerModifyLocalPhase::completed;
 else if(v["phase"]!="starting"){reason="invalid_native_timer_modify_state_phase";return false;}
 std::string payloadText;BackendAgentNativeTimerModifyPayload p;
 std::uint64_t claim=0,start=0,dispatch=0,completed=0;
 if(!unhex(v["payload_hex"],payloadText)||
    !backendAgentNativeTimerModifyParsePayload(payloadText,p,reason)||
    !number(v["claim_epoch"],claim)||!number(v["local_starting_persisted_at"],start)||
    !number(v["dispatch_started_at"],dispatch)||!number(v["completed_at"],completed)){
   reason="invalid_native_timer_modify_state_encoding";return false;}
 s.command=commandFor(v["command_id"],v["request_fingerprint"],v["operation_id"],
   v["job_id"],v["attempt_id"],claim,v["agent_id"],v["agent_instance_id"],p);
 s.localStartingPersistedAt=static_cast<std::int64_t>(start);
 if(s.phase==BackendAgentNativeTimerModifyLocalPhase::starting){
   if(v["outcome"]!="none"||dispatch||completed||!v["evidence_reference"].empty()){
     reason="invalid_native_timer_modify_starting_encoding";return false;}
 }else{
   BackendAgentNativeTimerModifyOutcomeCategory o;if(!parseOutcome(v["outcome"],o)){
     reason="invalid_native_timer_modify_outcome";return false;}
   s.evidence=evidenceFor(s.command,s.localStartingPersistedAt,o,
     static_cast<std::int64_t>(dispatch),static_cast<std::int64_t>(completed),v["evidence_reference"]);
 }
 if(!backendAgentNativeTimerModifyLocalStateValid(s,reason))return false;
 state=std::move(s);reason.clear();return true;
}
}
