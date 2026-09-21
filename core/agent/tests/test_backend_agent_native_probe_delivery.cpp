#include "AccountabilityEventRepository.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentNativeProbe.h"
#include "Database.h"

#include <cassert>
#include <string>

namespace
{
RequestSecurityContext context(ActorType type, const std::string& actor)
{
    RequestSecurityContext value;
    value.requestId = "req_native_test";
    value.correlationId = "corr_native_test";
    value.authenticationState = AuthenticationState::Authenticated;
    value.actor = ActorIdentity{actor, type, "test", true};
    value.device = DeviceIdentity{"dev_native_test", true};
    value.credential = CredentialIdentity{
        "cred_native_test", true, false, false};
    value.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return value;
}

vdrsuite::agent::BackendAgentLocalProviderFacts providerFacts()
{
    vdrsuite::agent::BackendAgentLocalProviderFacts facts;
    facts.providerId = "suitebridge:local";
    facts.providerKind = "suitebridge";
    facts.providerInstanceEpoch = "pie_native_1";
    facts.providerGeneration = 1;
    facts.capabilityRevision = 1;
    facts.available = true;
    facts.capabilities = {"vdr.native.probe"};
    return facts;
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
        "credential_id,credential_generation,agent_instance_id,"
        "backend_generation,protocol_version,software_version,"
        "heartbeat_sequence,capability_revision,last_connected_at,"
        "last_heartbeat_at,lease_expires_at,created_at,updated_at) VALUES("
        "'agt_native','default','actor_native','dev_native','cred_native',1,"
        "'inst_native',7,'vdr-suite-agent/1','test',2,1,100,100,1000,1,1);"));

    BackendAgentCommandDeliveryService service(
        commands, agents, accountability);
    std::string reason;
    const auto system = context(ActorType::System, "system_admin");
    const auto agent = context(ActorType::Agent, "actor_native");

    const auto unavailable = service.assignNativeProbe(
        system, "default", 100, 500, reason);
    assert(!unavailable.has_value());
    assert(reason == "native_probe_capability_required");

    BackendAgentCommandPollRequest capability;
    capability.backendId = "default";
    capability.agentInstanceId = "inst_native";
    capability.backendGeneration = 7;
    capability.supportedCommandTypes = {"vdr.native.probe"};
    capability.localProviders = {providerFacts()};
    const auto capabilityPoll = service.poll(agent, capability, 101);
    assert(capabilityPoll.accepted);
    assert(!capabilityPoll.assignment.present);

    const auto noOwnership = service.assignNativeProbe(
        system, "default", 102, 500, reason);
    assert(!noOwnership.has_value());
    assert(reason == "local_provider_ownership_required");

    vdrsuite::agent::BackendAgentLocalProviderOwnership ownership;
    assert(commands.setLocalProviderOwnership(
        "default", "vdr.native", "suitebridge:local", "suitebridge",
        {"vdr.native.probe"}, 103, ownership, reason));
    assert(reason == "local_provider_ownership_set");
    assert(ownership.ownershipGeneration == 1);

    const auto assignment = service.assignNativeProbe(
        system, "default", 104, 500, reason);
    assert(assignment.has_value());
    assert(assignment->commandType == "vdr.native.probe");
    assert(assignment->payloadVersion == 2);
    assert(assignment->verificationPolicy == "readback_required");
    vdrsuite::agent::BackendAgentNativeProbePayload payload;
    assert(vdrsuite::agent::backendAgentNativeProbeParseSelectedPayload(
        assignment->payload, payload, reason));
    assert(payload.localProviderSelection.providerId == "suitebridge:local");
    assert(payload.localProviderSelection.providerKind == "suitebridge");
    assert(payload.localProviderSelection.providerInstanceEpoch == "pie_native_1");
    assert(payload.localProviderSelection.providerGeneration == 1);
    assert(payload.localProviderSelection.capabilityRevision == 1);
    assert(payload.localProviderSelection.ownershipGeneration == 1);
    assert(payload.localProviderSelection.requiredCapability == "vdr.native.probe");
    assert(assignment->requestFingerprint ==
           backendAgentCommandFingerprint(*assignment));
    assert(backendAgentCommandValidAssignment(*assignment));

    const auto delivered = service.poll(agent, capability, 105);
    assert(delivered.accepted);
    assert(delivered.assignment.present);
    assert(delivered.assignment.commandId == assignment->commandId);
    return 0;
}
