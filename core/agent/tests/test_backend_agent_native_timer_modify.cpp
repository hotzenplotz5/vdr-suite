#include "../include/BackendAgentNativeTimerModifyExecutor.h"
#include "../include/BackendAgentNativeTimerModifyPayload.h"

#include <cstdlib>
#include <iostream>

using namespace vdrsuite::agent;
namespace {
void require(bool v,const char* m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
BackendAgentNativeTimerModifyPayload payload(){
 BackendAgentNativeTimerModifyPayload p;p.kind=BackendAgentNativeTimerModifyKind::update;
 p.operationRevision="1";p.timerAssignmentId="assign_1";p.expectedAssignmentRevision="1";
 p.expectedIntentRevision="1";p.assignmentEpoch=1;p.nativeTimerBindingId="bind_1";
 p.expectedBindingRevision="1";p.backendId="backend_1";p.backendGeneration=1;
 p.backendNativeTimerId="42";
 p.expectedCurrentFingerprint="sha256:aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
 p.specification.channelId="S19.2E-1-1019-10301";p.specification.title="Title";
 p.specification.day="2026-08-17";p.specification.startTime="1200";
 p.specification.endTime="1300";p.expectedSpecificationFingerprint=
   backendAgentNativeTimerCreateSpecificationFingerprint(p.specification);
 p.controlPlaneClaimedAt=100;auto& s=p.localProviderSelection;s.backendId=p.backendId;
 s.authorityDomain="vdr.timer";s.providerId="suitebridge:local";s.providerKind="suitebridge";
 s.ownershipGeneration=1;s.providerInstanceEpoch="pie_1";s.providerGeneration=1;
 s.capabilityRevision=1;s.requiredCapability="vdr.timer.update";return p;
}
BackendAgentCommandAssignment assignment(){
 BackendAgentCommandAssignment a;a.commandId="cmd_1";
 a.requestId="req_1";a.correlationId="req_1";
 a.operationId="op_1";a.commandType="vdr.timer.update";a.payloadVersion=1;
 a.payload=backendAgentNativeTimerModifyPayload(payload());a.verificationPolicy="readback_required";
 a.jobId="job_1";a.attemptId="attempt_1";a.claimEpoch=1;a.backendId="backend_1";
 a.agentId="agent_1";a.agentInstanceId="instance_1";a.backendGeneration=1;
 a.assignedAt=100;a.deadline=1000;a.requestFingerprint=backendAgentCommandFingerprint(a);
 return a;
}
struct Transport final:IBackendAgentNativeTimerModifyTransport{
 int calls=0;bool discoverProvider(BackendAgentLocalProviderFacts& f,std::string&)override{
  f.providerId="suitebridge:local";f.providerKind="suitebridge";f.providerInstanceEpoch="pie_1";
  f.providerGeneration=1;f.capabilityRevision=1;f.available=true;
  f.capabilities={"vdr.timer.update","vdr.timer.toggle"};return true;}
 BackendAgentNativeTimerModifyTransportReply modifyTimer(
   const BackendAgentNativeTimerModifyTransportRequest&)override{
  ++calls;return {BackendAgentNativeTimerModifyTransportDisposition::acceptedUnverified,
                  "ntmod:test:accepted"};}
};
}
int main(){
 auto boolPayload=payload();
 boolPayload.specification.enabled=false;
 boolPayload.specification.vps=true;
 boolPayload.expectedSpecificationFingerprint=
   backendAgentNativeTimerCreateSpecificationFingerprint(boolPayload.specification);
 const auto boolEncoded=backendAgentNativeTimerModifyPayload(boolPayload);
 require(!boolEncoded.empty(),"boolean payload serialization failed");
 require(boolEncoded.find("|6:update|")!=std::string::npos,
         "modify kind serialized through wrong overload");
 BackendAgentNativeTimerModifyPayload boolParsed;std::string boolReason;
 require(backendAgentNativeTimerModifyParsePayload(boolEncoded,boolParsed,boolReason),
         "boolean payload parse failed");
 require(!boolParsed.specification.enabled&&boolParsed.specification.vps,
         "boolean payload values did not roundtrip");

 auto togglePayload=payload();
 togglePayload.kind=BackendAgentNativeTimerModifyKind::toggle;
 togglePayload.localProviderSelection.requiredCapability="vdr.timer.toggle";
 const auto toggleEncoded=backendAgentNativeTimerModifyPayload(togglePayload);
 require(!toggleEncoded.empty()&&toggleEncoded.find("|6:toggle|")!=std::string::npos,
         "toggle kind serialization failed");
 BackendAgentNativeTimerModifyPayload toggleParsed;std::string toggleReason;
 require(backendAgentNativeTimerModifyParsePayload(toggleEncoded,toggleParsed,toggleReason)&&
         toggleParsed.kind==BackendAgentNativeTimerModifyKind::toggle,
         "toggle payload roundtrip failed");

 auto a=assignment();BackendAgentNativeTimerModifyLocalState state;std::string reason;
 require(backendAgentNativeTimerModifyPrepareLocalStarting(a,101,state,reason),"prepare failed");
 const auto encoded=backendAgentNativeTimerModifySerializeLocalState(state,reason);
 BackendAgentNativeTimerModifyLocalState parsed;
 require(!encoded.empty()&&backendAgentNativeTimerModifyParseLocalState(encoded,parsed,reason),
         "durable state roundtrip failed");
 auto recovery=backendAgentNativeTimerModifyRecoverLocalState(
   parsed,"backend_1","agent_1","instance_1",1,102);
 require(recovery.decision==BackendAgentNativeTimerModifyRecoveryDecision::reconcileOnly,
         "starting recovery may not retry");
 Transport transport;BackendAgentNativeTimerModifyEvidence evidence;
 BackendAgentNativeTimerModifyExecutorContext context{"backend_1","agent_1","instance_1",1,102};
 require(backendAgentNativeTimerModifyExecuteFreshStartingOnce(
   a,parsed,context,transport,evidence,reason)&&transport.calls==1&&
   evidence.outcome==BackendAgentNativeTimerModifyOutcomeCategory::acceptedUnverified,
   "fresh executor failed");
 require(backendAgentNativeTimerModifyCompleteLocalState(parsed,evidence,reason),
         "completion persist failed");
 std::cout<<"native timer modify Agent contract tests passed\n";
}
