#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

namespace
{
RequestSecurityContext context(ActorType type,const std::string& actor)
{
    RequestSecurityContext c;c.requestId="req_test";c.correlationId="corr_test";c.authenticationState=AuthenticationState::Authenticated;c.actor=ActorIdentity{actor,type,"test",true};c.device=DeviceIdentity{"dev_test",true};c.credential=CredentialIdentity{"cred_test",true,false,false};c.permissionGrantResolution=PermissionGrantResolutionState::Resolved;return c;
}
}
int main()
{
    Database db;assert(db.open(":memory:"));AccountabilityEventRepository accountability(db);BackendAgentRepository agents(db);BackendAgentCommandRepository commands(db);assert(accountability.ensureSchema());assert(agents.ensureSchema());assert(commands.ensureSchema());
    assert(db.execute("INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,credential_id,credential_generation,agent_instance_id,backend_generation,protocol_version,software_version,heartbeat_sequence,capability_revision,last_connected_at,last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES('agt_test','default','actor_agent','dev_agent','cred_agent',1,'inst_test',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));
    BackendAgentCommandDeliveryService service(commands,agents,accountability);std::string reason;
    auto unsupported=service.assignProbe(context(ActorType::System,"system_admin"),"default",100,500,reason);assert(!unsupported.has_value()&&reason=="command_capability_required");
    BackendAgentCommandPollRequest poll;poll.backendId="default";poll.agentInstanceId="inst_test";poll.backendGeneration=7;poll.supportedCommandTypes={"probe.noop"};auto capabilityPoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,101);assert(capabilityPoll.accepted&&!capabilityPoll.assignment.present);
    auto assignment=service.assignProbe(context(ActorType::System,"system_admin"),"default",102,500,reason);assert(assignment.has_value());assert(assignment->requestFingerprint==backendAgentCommandFingerprint(*assignment));
    auto polled=service.poll(context(ActorType::Agent,"actor_agent"),poll,103);assert(polled.accepted&&polled.assignment.present&&polled.assignment.commandId==assignment->commandId);
    auto duplicatePoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,102);assert(duplicatePoll.assignment.present&&duplicatePoll.assignment.commandId==assignment->commandId);
    BackendAgentCommandReceipt receipt;receipt.commandId=assignment->commandId;receipt.requestFingerprint=assignment->requestFingerprint;receipt.jobId=assignment->jobId;receipt.attemptId=assignment->attemptId;receipt.claimEpoch=1;receipt.backendId="default";receipt.agentId="agt_test";receipt.agentInstanceId="inst_test";receipt.backendGeneration=7;receipt.receiptCategory="accepted";receipt.receivedAt=103;receipt.reasonCode="durably_recorded";auto receiptResult=service.receipt(context(ActorType::Agent,"actor_agent"),receipt,103);assert(receiptResult.accepted&&!receiptResult.replayed);auto receiptReplay=service.receipt(context(ActorType::Agent,"actor_agent"),receipt,104);assert(receiptReplay.accepted&&receiptReplay.replayed);
    BackendAgentCommandResult result;result.commandId=assignment->commandId;result.requestFingerprint=assignment->requestFingerprint;result.jobId=assignment->jobId;result.attemptId=assignment->attemptId;result.claimEpoch=1;result.backendId="default";result.agentId="agt_test";result.agentInstanceId="inst_test";result.backendGeneration=7;result.dispatchState="effect_reported";result.verificationState="not_required";result.resultCategory="succeeded";result.errorCategory="none";result.retryClassification="none";result.boundedDiagnostics="probe.noop completed without native side effect";result.completedAt=105;auto resultAck=service.result(context(ActorType::Agent,"actor_agent"),result,105);assert(resultAck.accepted&&!resultAck.replayed);auto resultReplay=service.result(context(ActorType::Agent,"actor_agent"),result,106);assert(resultReplay.accepted&&resultReplay.replayed);
    auto summary=service.summaryForBackend("default");assert(summary.present&&summary.state=="completed"&&summary.resultCategory=="succeeded"&&summary.deliveryCount==2&&summary.receiptReplayCount==1&&summary.resultReplayCount==1);
    assert(service.requestReplay(context(ActorType::System,"system_admin"),"default",assignment->commandId,107,reason));auto replayPoll=service.poll(context(ActorType::Agent,"actor_agent"),poll,108);assert(replayPoll.assignment.present&&replayPoll.assignment.commandId==assignment->commandId);summary=service.summaryForBackend("default");assert(summary.deliveryCount==3);
    std::atomic<bool> pollStarted{false};
    std::atomic<bool> pollFinished{false};
    BackendAgentCommandPollResult concurrentPoll;
    auto transactionLease=db.acquireTransactionLease();
    assert(db.execute("BEGIN IMMEDIATE;"));
    std::thread pollThread([&] {
        pollStarted.store(true);
        concurrentPoll=service.poll(
            context(ActorType::Agent,"actor_agent"),poll,109);
        pollFinished.store(true);
    });
    while(!pollStarted.load())std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const bool pollBlocked=!pollFinished.load();
    assert(db.execute("COMMIT;"));
    transactionLease.unlock();
    pollThread.join();
    assert(
        pollBlocked&&pollFinished.load()&&
        concurrentPoll.accepted);
    BackendAgentCommandPollRequest stale=poll;stale.backendGeneration=6;assert(!service.poll(context(ActorType::Agent,"actor_agent"),stale,109).accepted);
    assert(service.armFault(context(ActorType::System,"system_admin"),"default","receipt",110,reason));
    std::cout<<"Backend Agent command delivery tests passed"<<std::endl;return 0;
}
