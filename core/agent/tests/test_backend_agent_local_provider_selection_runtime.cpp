#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeProbe.h"
#include "Database.h"

#include <cassert>
#include <string>

namespace
{
RequestSecurityContext context(ActorType type,const std::string& actor)
{
    RequestSecurityContext value;
    value.requestId="req_provider_runtime";
    value.correlationId="corr_provider_runtime";
    value.authenticationState=AuthenticationState::Authenticated;
    value.actor=ActorIdentity{actor,type,"test",true};
    value.device=DeviceIdentity{"dev_provider_runtime",true};
    value.credential=CredentialIdentity{"cred_provider_runtime",true,false,false};
    value.permissionGrantResolution=PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts suiteBridge(
    const std::string& epoch="pie_1",std::uint64_t revision=1)
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId="suitebridge:local";
    facts.providerKind="suitebridge";
    facts.providerInstanceEpoch=epoch;
    facts.providerGeneration=1;
    facts.capabilityRevision=revision;
    facts.available=true;
    facts.capabilities={"vdr.native.probe"};
    return facts;
}

vdrsuite::agent::BackendAgentLocalProviderFacts restful()
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId="restfulapi:local";
    facts.providerKind="restfulapi";
    facts.providerInstanceEpoch="rest_1";
    facts.providerGeneration=1;
    facts.capabilityRevision=1;
    facts.available=true;
    facts.capabilities={"vdr.native.probe"};
    return facts;
}

BackendAgentCommandPollRequest pollRequest(
    std::vector<vdrsuite::agent::BackendAgentLocalProviderFacts> providers)
{
    BackendAgentCommandPollRequest request;
    request.backendId="default";
    request.agentInstanceId="inst_provider";
    request.backendGeneration=9;
    request.supportedCommandTypes={"vdr.native.probe"};
    request.localProviders=std::move(providers);
    return request;
}

BackendAgentCommandReceipt receipt(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t receivedAt=200)
{
    BackendAgentCommandReceipt value;
    value.commandId=assignment.commandId;
    value.requestFingerprint=assignment.requestFingerprint;
    value.jobId=assignment.jobId;
    value.attemptId=assignment.attemptId;
    value.claimEpoch=assignment.claimEpoch;
    value.backendId=assignment.backendId;
    value.agentId=assignment.agentId;
    value.agentInstanceId=assignment.agentInstanceId;
    value.backendGeneration=assignment.backendGeneration;
    value.receiptCategory="accepted";
    value.receivedAt=receivedAt;
    value.reasonCode="durably_recorded";
    return value;
}

BackendAgentCommandResult result(
    const BackendAgentCommandAssignment& assignment,
    std::int64_t completedAt=210)
{
    BackendAgentCommandResult value;
    value.commandId=assignment.commandId;
    value.requestFingerprint=assignment.requestFingerprint;
    value.jobId=assignment.jobId;
    value.attemptId=assignment.attemptId;
    value.claimEpoch=assignment.claimEpoch;
    value.backendId=assignment.backendId;
    value.agentId=assignment.agentId;
    value.agentInstanceId=assignment.agentInstanceId;
    value.backendGeneration=assignment.backendGeneration;
    value.dispatchState="effect_reported";
    value.verificationState="verified";
    value.resultCategory="succeeded";
    value.errorCategory="none";
    value.retryClassification="none";
    value.boundedDiagnostics="selected provider verified";
    value.completedAt=completedAt;
    return value;
}

BackendAgentCommandAssignment legacyNativeAssignment()
{
    BackendAgentCommandAssignment value;
    value.present=true;
    value.requestId="req_legacy_provider";
    value.correlationId="corr_legacy_provider";
    value.operationId="op_legacy_provider";
    value.jobId="job_legacy_provider";
    value.attemptId="att_legacy_provider";
    value.claimEpoch=1;
    value.commandId="cmd_legacy_provider";
    value.backendId="default";
    value.agentId="agt_provider";
    value.agentInstanceId="inst_provider";
    value.backendGeneration=9;
    value.commandType="vdr.native.probe";
    value.payloadVersion=1;
    value.payload="{\"probeSchema\":1,\"probeNonce\":\"pbn_legacy_provider\"}";
    value.verificationPolicy="readback_required";
    value.assignedAt=120;
    value.deadline=500;
    value.requestFingerprint=backendAgentCommandFingerprint(value);
    assert(backendAgentCommandValidAssignment(value));
    return value;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    AccountabilityEventRepository accountability(database);
    BackendAgentRepository agents(database);
    BackendAgentCommandRepository commands(database);
    assert(accountability.ensureSchema());
    assert(agents.ensureSchema());
    assert(commands.ensureSchema());
    assert(database.execute(
        "INSERT INTO backend_agents(agent_id,backend_id,actor_id,device_id,"
        "credential_id,credential_generation,agent_instance_id,backend_generation,"
        "protocol_version,software_version,heartbeat_sequence,capability_revision,"
        "last_connected_at,last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_provider','default','actor_provider','dev_provider','cred_provider',1,"
        "'inst_provider',9,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));

    BackendAgentCommandDeliveryService service(commands,agents,accountability);
    const auto system=context(ActorType::System,"system_provider_admin");
    const auto agent=context(ActorType::Agent,"actor_provider");
    std::string reason;

    vdrsuite::agent::BackendAgentLocalProviderOwnership ownership;
    assert(commands.setLocalProviderOwnership(
        "default","vdr.native","suitebridge:local","suitebridge",
        {"vdr.native.probe"},100,ownership,reason));
    assert(reason=="local_provider_ownership_set");
    assert(ownership.ownershipGeneration==1);
    vdrsuite::agent::BackendAgentLocalProviderOwnership unchanged;
    assert(commands.setLocalProviderOwnership(
        "default","vdr.native","suitebridge:local","suitebridge",
        {"vdr.native.probe"},101,unchanged,reason));
    assert(reason=="local_provider_ownership_unchanged");
    assert(unchanged.ownershipGeneration==1);

    // An available alternate provider is descriptive only. It never becomes a
    // continuation or assignment fallback for SuiteBridge ownership.
    auto alternateOnly=pollRequest({restful()});
    assert(service.poll(agent,alternateOnly,102).accepted);
    const auto noFallback=service.assignNativeProbe(
        system,"default",103,500,reason);
    assert(!noFallback.has_value());
    assert(reason=="local_provider_facts_required");

    auto current=pollRequest({suiteBridge(),restful()});
    const auto currentPoll=service.poll(agent,current,104);
    assert(currentPoll.accepted);
    assert(!currentPoll.assignment.present);
    const auto assignment=service.assignNativeProbe(
        system,"default",105,500,reason);
    assert(assignment.has_value());
    assert(assignment->payloadVersion==2);
    vdrsuite::agent::BackendAgentNativeProbePayload payload;
    assert(vdrsuite::agent::backendAgentNativeProbeParseSelectedPayload(
        assignment->payload,payload,reason));
    assert(payload.localProviderSelection.providerId=="suitebridge:local");
    assert(payload.localProviderSelection.providerKind=="suitebridge");
    assert(payload.localProviderSelection.providerInstanceEpoch=="pie_1");
    assert(payload.localProviderSelection.ownershipGeneration==1);
    assert(commands.localProviderSelectionCurrent(assignment->commandId,reason));
    assert(reason=="local_provider_selection_current");

    // Provider epoch replacement fences delivery and receipt before local dispatch.
    auto replaced=pollRequest({suiteBridge("pie_2"),restful()});
    const auto replacedPoll=service.poll(agent,replaced,106);
    assert(replacedPoll.accepted);
    assert(!replacedPoll.assignment.present);
    const auto staleReceipt=service.receipt(agent,receipt(*assignment),107);
    assert(!staleReceipt.accepted);
    assert(staleReceipt.reasonCode=="local_provider_selection_stale");

    // Restore the exact facts and the original assignment becomes deliverable.
    const auto restoredPoll=service.poll(agent,current,108);
    assert(restoredPoll.accepted);
    assert(restoredPoll.assignment.present);
    assert(restoredPoll.assignment.commandId==assignment->commandId);
    const auto acceptedReceipt=service.receipt(agent,receipt(*assignment),109);
    assert(acceptedReceipt.accepted);

    // Ownership revocation fences all further dispatch/replay authorization.
    // Already-produced outcome evidence remains ingestible by immutable command
    // identity so reconciliation never loses what actually happened.
    assert(commands.clearLocalProviderOwnership(
        "default","vdr.native",110,reason));
    const auto cleared=commands.localProviderOwnershipStatus(
        "default","vdr.native");
    assert(cleared.present&&!cleared.active);
    assert(cleared.ownership.ownershipGeneration==2);
    assert(!commands.localProviderSelectionCurrent(assignment->commandId,reason));
    assert(reason=="local_provider_ownership_required");
    const auto staleDuplicateReceipt=service.receipt(
        agent,receipt(*assignment),111);
    assert(!staleDuplicateReceipt.accepted);
    assert(staleDuplicateReceipt.reasonCode=="local_provider_ownership_required");
    const auto preservedResult=service.result(agent,result(*assignment),112);
    assert(preservedResult.accepted);
    assert(preservedResult.reasonCode=="command_result_accepted");

    vdrsuite::agent::BackendAgentLocalProviderOwnership restoredOwnership;
    assert(commands.setLocalProviderOwnership(
        "default","vdr.native","suitebridge:local","suitebridge",
        {"vdr.native.probe"},113,restoredOwnership,reason));
    assert(restoredOwnership.ownershipGeneration==3);
    assert(!commands.localProviderSelectionCurrent(assignment->commandId,reason));
    assert(reason=="local_provider_selection_stale");
    assert(service.requestReplay(
        system,"default",assignment->commandId,114,reason));
    const auto fencedReplay=service.poll(agent,current,115);
    assert(fencedReplay.accepted);
    assert(!fencedReplay.assignment.present);

    // Capability revision replacement is another independent fence.
    const auto newAssignment=service.assignNativeProbe(
        system,"default",116,500,reason);
    assert(newAssignment.has_value());
    auto changedRevision=pollRequest({suiteBridge("pie_1",2)});
    const auto changedPoll=service.poll(agent,changedRevision,117);
    assert(changedPoll.accepted);
    assert(!changedPoll.assignment.present);
    assert(!commands.localProviderSelectionCurrent(newAssignment->commandId,reason));
    assert(reason=="local_provider_selection_stale");

    // Restoring the exact provider facts re-authorizes only the same immutable
    // v2 selection. Complete it before isolating the legacy-v1 delivery check.
    const auto restoredRevisionPoll=service.poll(agent,current,118);
    assert(restoredRevisionPoll.accepted);
    assert(restoredRevisionPoll.assignment.present);
    assert(restoredRevisionPoll.assignment.commandId==newAssignment->commandId);
    const auto newReceipt=service.receipt(agent,receipt(*newAssignment,119),119);
    assert(newReceipt.accepted);
    const auto newResult=service.result(agent,result(*newAssignment,120),120);
    assert(newResult.accepted);

    // Pre-upgrade v1 commands have no selection sidecar. New Control Plane poll
    // never delivers them, but already-local durable receipt/result evidence is
    // still accepted for reconciliation on the original command identity.
    const auto legacy=legacyNativeAssignment();
    assert(commands.insertAssignment(legacy));
    const auto noLegacyDelivery=service.poll(agent,current,121);
    assert(noLegacyDelivery.accepted);
    assert(!noLegacyDelivery.assignment.present);
    const auto legacyReceipt=service.receipt(agent,receipt(legacy,122),122);
    assert(legacyReceipt.accepted);
    const auto legacyResult=service.result(agent,result(legacy,123),123);
    assert(legacyResult.accepted);
    assert(legacyResult.reasonCode=="command_result_accepted");

    return 0;
}
