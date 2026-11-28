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
 int calls=0;bool changedProvider=false;bool discoverProvider(BackendAgentLocalProviderFacts& f,std::string&)override{
  f.providerId="suitebridge:local";f.providerKind="suitebridge";f.providerInstanceEpoch=changedProvider?"pie_changed":"pie_1";
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

 // Optional Aux mutation is durable in the already-existing Agent
 // assignment/local starting state, without changing the v1 payload.
 auto auxPayload=payload();
 auxPayload.modifyAux=true;
 auxPayload.expectedAuxFingerprint=
   "sha256:cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc";
 auxPayload.desiredAux="<epgsearch><searchtimer>42</searchtimer></epgsearch>";
 const auto auxEncoded=backendAgentNativeTimerModifyPayload(auxPayload);
 require(auxEncoded.find("native-timer-modify-agent-payload/2|")==0,
         "Aux Agent v2 serialization missing");
 BackendAgentNativeTimerModifyPayload auxParsed;std::string auxReason;
 require(backendAgentNativeTimerModifyParsePayload(
    auxEncoded,auxParsed,auxReason)&&auxParsed.modifyAux &&
    auxParsed.expectedAuxFingerprint==auxPayload.expectedAuxFingerprint &&
    auxParsed.desiredAux==auxPayload.desiredAux,
    "Aux Agent v2 payload did not roundtrip");
 auto invalidAux=auxPayload;
 invalidAux.kind=BackendAgentNativeTimerModifyKind::toggle;
 require(backendAgentNativeTimerModifyPayload(invalidAux).empty(),
         "Aux mutation must never use timer.toggle");
 auto auxAssignment=assignment();
 auxAssignment.payloadVersion=2;
 auxAssignment.payload=auxEncoded;
 auxAssignment.requestFingerprint=backendAgentCommandFingerprint(auxAssignment);
 BackendAgentNativeTimerModifyLocalState auxLocal;
 require(backendAgentNativeTimerModifyPrepareLocalStarting(
    auxAssignment,101,auxLocal,auxReason),
    "Aux Agent starting state was not accepted");
 const auto auxLocalSerialized=backendAgentNativeTimerModifySerializeLocalState(
    auxLocal,auxReason);
 BackendAgentNativeTimerModifyLocalState auxRecovered;
 require(!auxLocalSerialized.empty() &&
    backendAgentNativeTimerModifyParseLocalState(
        auxLocalSerialized,auxRecovered,auxReason) &&
    auxRecovered.command.modifyAux &&
    auxRecovered.command.desiredAux==auxPayload.desiredAux &&
    auxRecovered.command.expectedAuxFingerprint==auxPayload.expectedAuxFingerprint,
    "Aux Agent starting state lost compare-and-write evidence");
 require(backendAgentNativeTimerModifyRecoverLocalState(
    auxRecovered,"backend_1","agent_1","instance_1",1,102).decision ==
    BackendAgentNativeTimerModifyRecoveryDecision::reconcileOnly,
    "uncertain Aux recovery must not dispatch a second write");
 auto forgedAux=auxLocalSerialized;
 const auto beforeAux=forgedAux.find("payload_hex=");
 require(beforeAux!=std::string::npos,"Aux crash state test input missing");
 forgedAux.replace(beforeAux+12,2,"ff");
 require(!backendAgentNativeTimerModifyParseLocalState(
    forgedAux,auxRecovered,auxReason),
    "forged Aux crash state should never recover a write");

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
 const auto resultEvidence =
    backendAgentNativeTimerModifyResultEvidence(evidence,a,reason);
 require(!resultEvidence.empty(),"MODIFY result serialization rejected");
 BackendAgentNativeTimerModifyEvidence restoredEvidence;
 require(backendAgentNativeTimerModifyParseResultEvidence(
    resultEvidence,a,restoredEvidence,reason),
    "MODIFY result evidence did not roundtrip");
 require(restoredEvidence.dispatchStartedAt==evidence.dispatchStartedAt &&
         restoredEvidence.localStartingPersistedAt==evidence.localStartingPersistedAt &&
         restoredEvidence.operationId==evidence.operationId &&
         restoredEvidence.evidenceReference==evidence.evidenceReference,
         "MODIFY result lost immutable native dispatch identity");
 auto forgedEvidence=resultEvidence;
 forgedEvidence.push_back('x');
 require(!backendAgentNativeTimerModifyParseResultEvidence(
    forgedEvidence,a,restoredEvidence,reason),
    "MODIFY parser accepted forged/trailing evidence");
 // A changed local provider must fence execution before any native write,
 // but still yield structured evidence so the daemon can finish safely.
 BackendAgentNativeTimerModifyLocalState fencedState;
 require(backendAgentNativeTimerModifyPrepareLocalStarting(
    a,105,fencedState,reason),"fenced prepare failed");
 Transport changed;changed.changedProvider=true;
 BackendAgentNativeTimerModifyEvidence fencedEvidence;
 BackendAgentNativeTimerModifyExecutorContext fencedContext{
    "backend_1","agent_1","instance_1",1,106};
 require(backendAgentNativeTimerModifyExecuteFreshStartingOnce(
    a,fencedState,fencedContext,changed,fencedEvidence,reason) &&
    changed.calls==0 &&
    fencedEvidence.outcome==
      BackendAgentNativeTimerModifyOutcomeCategory::rejectedWithoutEffect &&
    fencedEvidence.dispatchStartedAt==0 &&
    fencedEvidence.evidenceReference=="executor-fence:provider-changed",
    "provider fence must reject without executor call");
 const auto fencedResultEvidence=backendAgentNativeTimerModifyResultEvidence(
    fencedEvidence,a,reason);
 require(!fencedResultEvidence.empty() &&
    backendAgentNativeTimerModifyParseResultEvidence(
       fencedResultEvidence,a,restoredEvidence,reason) &&
    restoredEvidence.evidenceReference=="executor-fence:provider-changed",
    "fenced MODIFY must provide canonical durable result evidence");
 require(!backendAgentNativeTimerModifyParseResultEvidence(
    "",a,restoredEvidence,reason),
    "empty MODIFY result evidence must fail closed");
 std::cout<<"native timer modify Agent contract tests passed\n";
}
