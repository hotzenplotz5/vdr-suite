#include "BackendAgentNativeTimerModifyAssignment.h"

#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeTimerDeleteFingerprint.h"
#include "BackendAgentNativeTimerModifyPayload.h"
#include "Database.h"

#include <optional>

namespace vdrsuite::agent
{
namespace {
const char* commandType(BackendAgentNativeTimerModifyKind kind) {
 return kind==BackendAgentNativeTimerModifyKind::toggle
   ? kBackendAgentNativeTimerToggleCommandType:kBackendAgentNativeTimerUpdateCommandType;
}
BackendAgentNativeTimerModifyAssignmentResult rejected(const std::string& reason) {
 BackendAgentNativeTimerModifyAssignmentResult r;r.reasonCode=reason;return r;
}
bool requestValid(const RequestSecurityContext& context,
 const BackendAgentNativeTimerModifyAssignmentRequest& r,
 std::int64_t now,std::int64_t deadline) {
 return context.authenticated()&&context.actor.type==ActorType::System&&
  backendAgentCommandSafeIdentifier(r.operationId)&&
  backendAgentCommandSafeIdentifier(r.operationRevision)&&
  backendAgentCommandSafeIdentifier(r.timerAssignmentId)&&
  backendAgentCommandSafeIdentifier(r.expectedAssignmentRevision)&&
  backendAgentCommandSafeIdentifier(r.expectedIntentRevision)&&r.assignmentEpoch>0&&
  backendAgentCommandSafeIdentifier(r.nativeTimerBindingId)&&
  backendAgentCommandSafeIdentifier(r.expectedBindingRevision)&&
  backendAgentCommandSafeIdentifier(r.backendId)&&r.backendGeneration>0&&
  backendAgentCommandSafeIdentifier(r.backendNativeTimerId)&&
  backendAgentNativeTimerDeleteCanonicalFingerprintValid(r.expectedCurrentFingerprint)&&
  backendAgentNativeTimerCreateSpecificationValid(r.specification)&&
  r.expectedSpecificationFingerprint==
    backendAgentNativeTimerCreateSpecificationFingerprint(r.specification)&&
  r.controlPlaneClaimedAt>0&&r.controlPlaneClaimedAt<=now&&now>0&&
  deadline>now&&deadline-now<=3600;
}
BackendAgentNativeTimerModifyPayload payloadFor(
 const BackendAgentNativeTimerModifyAssignmentRequest& r,
 const BackendAgentLocalProviderSelection& selection) {
 BackendAgentNativeTimerModifyPayload p;p.kind=r.kind;p.operationRevision=r.operationRevision;
 p.timerAssignmentId=r.timerAssignmentId;p.expectedAssignmentRevision=r.expectedAssignmentRevision;
 p.expectedIntentRevision=r.expectedIntentRevision;p.assignmentEpoch=r.assignmentEpoch;
 p.nativeTimerBindingId=r.nativeTimerBindingId;p.expectedBindingRevision=r.expectedBindingRevision;
 p.backendId=r.backendId;p.backendGeneration=r.backendGeneration;
 p.backendNativeTimerId=r.backendNativeTimerId;
 p.expectedCurrentFingerprint=backendAgentNativeTimerDeleteFingerprintToken(
    r.expectedCurrentFingerprint);
 p.expectedSpecificationFingerprint=r.expectedSpecificationFingerprint;
 p.specification=r.specification;p.controlPlaneClaimedAt=r.controlPlaneClaimedAt;
 p.localProviderSelection=selection;return p;
}
BackendAgentNativeTimerModifyCommand domainCommand(
 const BackendAgentCommandAssignment& a,const BackendAgentNativeTimerModifyPayload& p) {
 BackendAgentNativeTimerModifyCommand c;c.kind=p.kind;c.commandId=a.commandId;
 c.requestFingerprint=a.requestFingerprint;c.operationId=a.operationId;
 c.operationRevision=p.operationRevision;c.timerAssignmentId=p.timerAssignmentId;
 c.expectedAssignmentRevision=p.expectedAssignmentRevision;
 c.expectedIntentRevision=p.expectedIntentRevision;c.assignmentEpoch=p.assignmentEpoch;
 c.nativeTimerBindingId=p.nativeTimerBindingId;c.expectedBindingRevision=p.expectedBindingRevision;
 c.backendNativeTimerId=p.backendNativeTimerId;c.expectedCurrentFingerprint=p.expectedCurrentFingerprint;
 c.expectedSpecificationFingerprint=p.expectedSpecificationFingerprint;c.specification=p.specification;
 c.jobId=a.jobId;c.attemptId=a.attemptId;c.claimEpoch=a.claimEpoch;c.backendId=a.backendId;
 c.agentId=a.agentId;c.agentInstanceId=a.agentInstanceId;c.backendGeneration=a.backendGeneration;
 c.controlPlaneClaimedAt=p.controlPlaneClaimedAt;c.localProviderSelection=p.localProviderSelection;
 return c;
}
bool exactRequest(const BackendAgentNativeTimerModifyAssignmentRequest& r,
 const BackendAgentCommandAssignment& a,const BackendAgentNativeTimerModifyPayload& p) {
 return a.commandType==commandType(r.kind)&&a.payloadVersion==1&&
  a.verificationPolicy=="readback_required"&&a.operationId==r.operationId&&
  a.backendId==r.backendId&&a.backendGeneration==r.backendGeneration&&
  p.kind==r.kind&&p.operationRevision==r.operationRevision&&
  p.timerAssignmentId==r.timerAssignmentId&&
  p.expectedAssignmentRevision==r.expectedAssignmentRevision&&
  p.expectedIntentRevision==r.expectedIntentRevision&&p.assignmentEpoch==r.assignmentEpoch&&
  p.nativeTimerBindingId==r.nativeTimerBindingId&&
  p.expectedBindingRevision==r.expectedBindingRevision&&
  p.backendNativeTimerId==r.backendNativeTimerId&&
  p.expectedCurrentFingerprint==backendAgentNativeTimerDeleteFingerprintToken(r.expectedCurrentFingerprint)&&
  p.expectedSpecificationFingerprint==r.expectedSpecificationFingerprint&&
  p.controlPlaneClaimedAt==r.controlPlaneClaimedAt;
}
bool exactExisting(BackendAgentCommandRepository& repository,
 const BackendAgentRecord& agent,const BackendAgentNativeTimerModifyAssignmentRequest& r,
 const BackendAgentCommandAssignment& a,std::string& reason) {
 BackendAgentNativeTimerModifyPayload p;
 if(!backendAgentNativeTimerModifyParsePayload(a.payload,p,reason)||
    !exactRequest(r,a,p)||!backendAgentCommandValidAssignment(a)){
  reason="native_timer_modify_assignment_conflict";return false;}
 if(a.agentId!=agent.agentId||a.agentInstanceId!=agent.agentInstanceId||
    a.backendGeneration!=agent.backendGeneration){
  reason="native_timer_modify_agent_fence_stale";return false;}
 const auto recorded=repository.localProviderSelectionForCommand(a.commandId);
 if(!recorded.has_value()||
    !backendAgentLocalProviderSameFence(*recorded,p.localProviderSelection)){
  reason="native_timer_modify_provider_selection_missing";return false;}
 const auto current=repository.selectLocalProvider(r.backendId,agent.agentId,
  agent.agentInstanceId,agent.backendGeneration,kBackendAgentNativeTimerModifyAuthorityDomain,
  backendAgentNativeTimerModifyCapability(r.kind),reason);
 if(!current.has_value()||!backendAgentLocalProviderSameFence(*recorded,*current)){
  reason="native_timer_modify_provider_selection_stale";return false;}
 if(!backendAgentNativeTimerModifyValidCommand(domainCommand(a,p),reason)){
  reason="native_timer_modify_assignment_contract_invalid";return false;}
 return true;
}
}

bool BackendAgentCommandRepository::ensureNativeTimerModifyAssignmentSchema()
{
 auto lease=database_.acquireTransactionLease();(void)lease;
 if(!database_.execute("BEGIN IMMEDIATE;"))return false;
 const bool ok=database_.execute(
  "CREATE UNIQUE INDEX IF NOT EXISTS idx_backend_agent_timer_modify_operation "
  "ON backend_agent_commands(backend_id,operation_id) "
  "WHERE command_type IN ('vdr.timer.update','vdr.timer.toggle');")&&
 database_.execute(
  "DELETE FROM backend_agent_command_capabilities "
  "WHERE command_type IN ('vdr.timer.update','vdr.timer.toggle');")&&
 database_.execute(
  "CREATE TRIGGER IF NOT EXISTS trg_backend_agent_timer_update_dormant_capability "
  "BEFORE INSERT ON backend_agent_command_capabilities "
  "WHEN NEW.command_type='vdr.timer.update' BEGIN SELECT RAISE(IGNORE); END;")&&
 database_.execute(
  "CREATE TRIGGER IF NOT EXISTS trg_backend_agent_timer_toggle_dormant_capability "
  "BEFORE INSERT ON backend_agent_command_capabilities "
  "WHEN NEW.command_type='vdr.timer.toggle' BEGIN SELECT RAISE(IGNORE); END;");
 if(!ok||!database_.execute("COMMIT;")){database_.execute("ROLLBACK;");return false;}
 return true;
}

BackendAgentNativeTimerModifyAssignmentService::
BackendAgentNativeTimerModifyAssignmentService(
 BackendAgentCommandRepository& commands,BackendAgentRepository& agents)
 :commandRepository_(commands),agentRepository_(agents){}

BackendAgentNativeTimerModifyAssignmentResult
BackendAgentNativeTimerModifyAssignmentService::assign(
 const RequestSecurityContext& context,
 const BackendAgentNativeTimerModifyAssignmentRequest& request,
 std::int64_t now,std::int64_t deadline)
{
 if(!requestValid(context,request,now,deadline))
  return rejected("invalid_native_timer_modify_assignment_request");
 const auto agent=agentRepository_.findAgentForBackend(request.backendId);
 if(!agent.has_value()||agent->revoked||agent->incompatible||
    agent->agentInstanceId.empty()||agent->backendGeneration==0||
    agent->leaseExpiresAt<now)return rejected("active_agent_lease_required");
 if(agent->backendGeneration!=request.backendGeneration)
  return rejected("native_timer_modify_backend_generation_conflict");
 if(!commandRepository_.ensureNativeTimerModifyAssignmentSchema())
  return rejected("native_timer_modify_assignment_schema_failed");
 const char* type=commandType(request.kind);
 if(const auto existing=commandRepository_.findAssignmentForOperation(
      request.backendId,request.operationId,type);existing.has_value()){
  std::string reason;if(!exactExisting(commandRepository_,*agent,request,*existing,reason))
    return rejected(reason);
  BackendAgentNativeTimerModifyAssignmentResult r;r.accepted=true;r.replayed=true;
  r.reasonCode="native_timer_modify_assignment_replayed";r.assignment=*existing;return r;
 }
 std::string reason;const auto selection=commandRepository_.selectLocalProvider(
  request.backendId,agent->agentId,agent->agentInstanceId,agent->backendGeneration,
  kBackendAgentNativeTimerModifyAuthorityDomain,
  backendAgentNativeTimerModifyCapability(request.kind),reason);
 if(!selection.has_value())return rejected(reason);
 const auto payload=payloadFor(request,*selection);
 BackendAgentCommandAssignment a;a.present=true;
 a.requestId=backendAgentGenerateOpaqueId("req_",8);a.correlationId=a.requestId;
 a.operationId=request.operationId;a.jobId=backendAgentGenerateOpaqueId("job_",12);
 a.attemptId=backendAgentGenerateOpaqueId("att_",12);a.claimEpoch=1;
 a.commandId=backendAgentGenerateOpaqueId("cmd_",12);a.backendId=request.backendId;
 a.agentId=agent->agentId;a.agentInstanceId=agent->agentInstanceId;
 a.backendGeneration=agent->backendGeneration;a.commandType=type;a.payloadVersion=1;
 a.payload=backendAgentNativeTimerModifyPayload(payload);
 a.verificationPolicy="readback_required";a.assignedAt=now;a.deadline=deadline;
 a.requestFingerprint=backendAgentCommandFingerprint(a);
 if(a.payload.empty()||!backendAgentNativeTimerModifyValidCommand(domainCommand(a,payload),reason)||
    !backendAgentCommandValidAssignment(a))
  return rejected("native_timer_modify_assignment_contract_invalid");
 if(commandRepository_.insertAssignment(a,&*selection)){
  BackendAgentNativeTimerModifyAssignmentResult r;r.accepted=true;
  r.reasonCode="native_timer_modify_assigned";r.assignment=a;return r;
 }
 const auto raced=commandRepository_.findAssignmentForOperation(
  request.backendId,request.operationId,type);
 if(raced.has_value()&&exactExisting(commandRepository_,*agent,request,*raced,reason)){
  BackendAgentNativeTimerModifyAssignmentResult r;r.accepted=true;r.replayed=true;
  r.reasonCode="native_timer_modify_assignment_replayed";r.assignment=*raced;return r;
 }
 return rejected(raced.has_value()?reason:"native_timer_modify_assignment_persist_failed");
}
}
