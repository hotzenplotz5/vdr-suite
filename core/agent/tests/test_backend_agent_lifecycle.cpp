#include "AccountabilityEventRepository.h"
#include "BackendAgentHttpServer.h"
#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "BackendRegistry.h"
#include "BackendRegistryService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>

#include <sqlite3.h>

namespace
{
constexpr std::int64_t Now = 1000000;
const std::string EnrollmentToken =
    "enrollment-token-material-00000000000000000000000000000001";
const std::string AgentSecret =
    "agent-runtime-secret-material-000000000000000000000000000001";
const std::string RotatedAgentSecret =
    "agent-rotated-secret-material-00000000000000000000000000001";

struct Fixture
{
    std::string path;
    Database database;
    BackendRegistry backendRegistry;
    BackendRegistryService backendRegistryService;
    BackendAgentRepository agentRepository;
    SecurityIdentityRepository identityRepository;
    SecurityIdentityProvisioningRepository provisioningRepository;
    CredentialVerifierRepository verifierRepository;
    AccountabilityEventRepository accountabilityRepository;
    BackendAgentCommandRepository commandRepository;
    BackendAgentLifecycleService service;
    BackendAgentCommandDeliveryService commandService;

    Fixture()
        : path("/tmp/vdr-suite-backend-agent-test.db"),
          backendRegistryService(backendRegistry),
          agentRepository(database),
          identityRepository(database),
          provisioningRepository(database),
          verifierRepository(database),
          accountabilityRepository(database),
          commandRepository(database),
          service(
              database,
              agentRepository,
              backendRegistryService,
              provisioningRepository,
              identityRepository,
              verifierRepository,
              accountabilityRepository),
          commandService(
              commandRepository,
              agentRepository,
              accountabilityRepository)
    {
        std::remove(path.c_str());
        assert(database.open(path));
        assert(identityRepository.ensureSchema());
        assert(verifierRepository.ensureSchema());
        assert(accountabilityRepository.ensureSchema());
        assert(agentRepository.ensureSchema());
        assert(commandRepository.ensureSchema());

        BackendNode backend;
        backend.backendId = "default";
        backend.backendName = "Default VDR";
        backend.accessMode = "read-only";
        backend.enabled = true;
        backendRegistry.addBackend(backend);

        BackendNode second;
        second.backendId = "ferienhaus";
        second.backendName = "Ferienhaus";
        second.accessMode = "read-only";
        second.enabled = true;
        backendRegistry.addBackend(second);
    }

    ~Fixture()
    {
        database.close();
        std::remove(path.c_str());
    }
};

RequestSecurityContext adminContext(const std::string& backendId = "default")
{
    RequestSecurityContext context;
    context.requestId = "request-admin";
    context.correlationId = "correlation-admin";
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{"admin", ActorType::User, "Administrator", true};
    context.grants.push_back(PermissionGrant{"role.admin", backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

RequestSecurityContext agentContext(
    const BackendAgentRecord& agent,
    AuthenticationState state = AuthenticationState::Authenticated)
{
    RequestSecurityContext context;
    context.requestId = "request-agent";
    context.correlationId = "correlation-agent";
    context.authenticationState = state;
    context.actor = ActorIdentity{agent.actorId, ActorType::Agent, "Backend Agent", true};
    context.device = DeviceIdentity{agent.deviceId, true};
    context.credential = CredentialIdentity{agent.credentialId, true, false, false};
    context.grants.push_back(PermissionGrant{
        "backend.agent.credential.rotate", agent.backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

BackendAgentEnrollmentMaterial createAndConsumeEnrollment(Fixture& fixture)
{
    const std::string tokenHash = backendAgentHashSecret(EnrollmentToken);
    assert(!tokenHash.empty());
    std::string reason;
    assert(fixture.service.createEnrollment(
        adminContext(), "enr_test_1", "default", tokenHash,
        Now + 600, Now, reason));
    assert(reason == "enrollment_created");

    BackendAgentEnrollmentMaterial enrollment = fixture.service.enroll(
        "enr_test_1", EnrollmentToken, AgentSecret, Now + 1);
    assert(enrollment.accepted);
    assert(!enrollment.idempotent);
    assert(enrollment.backendId == "default");
    assert(!enrollment.agentId.empty());
    assert(!enrollment.credentialId.empty());
    return enrollment;
}

BackendAgentConnectResult connectAgent(
    Fixture& fixture,
    const BackendAgentRecord& agent,
    const std::string& instanceId = "instance-1")
{
    BackendAgentConnectRequest request;
    request.backendId = agent.backendId;
    request.agentInstanceId = instanceId;
    request.protocolVersion = "vdr-suite-agent/1";
    request.softwareVersion = "vdr-suite-agent-test/1";
    BackendAgentConnectResult result = fixture.service.connect(
        agentContext(agent), request, Now + 2);
    assert(result.accepted);
    return result;
}

std::int64_t rowCount(Database& database, const std::string& table)
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql = "SELECT COUNT(*) FROM " + table + ";";
    assert(sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const std::int64_t value = sqlite3_column_int64(statement, 0);
    sqlite3_finalize(statement);
    return value;
}

void test_domain_values_and_capability_validation()
{
    assert(BackendAgentLifecycleService::safeIdentifier("agt_1234-abc.def"));
    assert(!BackendAgentLifecycleService::safeIdentifier(""));
    assert(!BackendAgentLifecycleService::safeIdentifier("agent secret"));
    assert(BackendAgentLifecycleService::supportedProtocol("vdr-suite-agent/1"));
    assert(!BackendAgentLifecycleService::supportedProtocol("vdr-suite-agent/2"));

    BackendAgentCapabilityFacts facts;
    facts.adapters = {"suitebridge", "restfulapi"};
    facts.observationDomains = {"backend-health", "recordings"};
    assert(BackendAgentLifecycleService::validCapabilities(facts));
    facts.readOnly = false;
    assert(!BackendAgentLifecycleService::validCapabilities(facts));
    facts.readOnly = true;
    facts.adapters = {"private-provider-url"};
    assert(!BackendAgentLifecycleService::validCapabilities(facts));
}

void test_enrollment_authorization_idempotency_and_binding()
{
    Fixture fixture;
    const std::string hash = backendAgentHashSecret(EnrollmentToken);
    std::string reason;
    RequestSecurityContext denied = adminContext();
    denied.grants.clear();
    assert(!fixture.service.createEnrollment(
        denied, "enr_denied", "default", hash, Now + 600, Now, reason));
    assert(reason == "permission_denied");

    assert(fixture.service.createEnrollment(
        adminContext(), "enr_test", "default", hash, Now + 600, Now, reason));
    assert(fixture.service.createEnrollment(
        adminContext(), "enr_test", "default", hash, Now + 600, Now, reason));
    assert(reason == "enrollment_exists");

    BackendAgentEnrollmentMaterial invalid = fixture.service.enroll(
        "enr_test", "wrong-enrollment-token-material-00000000000000000000",
        AgentSecret, Now + 1);
    assert(!invalid.accepted);
    assert(invalid.reasonCode == "invalid_enrollment_credentials");

    assert(fixture.service.createEnrollment(
        adminContext(), "enr_revoked", "default", hash, Now + 600, Now, reason));
    assert(fixture.service.revokeEnrollment(
        adminContext(), "enr_revoked", "operator-cancelled", Now + 1, reason));
    assert(reason == "enrollment_revoked");
    assert(fixture.service.revokeEnrollment(
        adminContext(), "enr_revoked", "operator-cancelled", Now + 2, reason));
    BackendAgentEnrollmentMaterial revokedEnrollment = fixture.service.enroll(
        "enr_revoked", EnrollmentToken, AgentSecret, Now + 2);
    assert(!revokedEnrollment.accepted);
    assert(revokedEnrollment.reasonCode == "enrollment_revoked");
    const auto revokedRecord = fixture.agentRepository.findEnrollment("enr_revoked");
    assert(revokedRecord.has_value());
    assert(revokedRecord->status == "revoked");

    BackendAgentEnrollmentMaterial first = fixture.service.enroll(
        "enr_test", EnrollmentToken, AgentSecret, Now + 1);
    assert(first.accepted);
    BackendAgentEnrollmentMaterial repeated = fixture.service.enroll(
        "enr_test", EnrollmentToken, AgentSecret, Now + 2);
    assert(repeated.accepted);
    assert(repeated.idempotent);
    assert(repeated.agentId == first.agentId);
    assert(repeated.credentialId == first.credentialId);

    BackendAgentEnrollmentMaterial conflicting = fixture.service.enroll(
        "enr_test", EnrollmentToken,
        "different-agent-secret-material-0000000000000000000000000001",
        Now + 3);
    assert(!conflicting.accepted);
    assert(conflicting.reasonCode == "enrollment_already_consumed");

    const auto agent = fixture.agentRepository.findAgent(first.agentId);
    assert(agent.has_value());
    assert(agent->backendId == "default");
    const auto actor = fixture.identityRepository.findActor(agent->actorId);
    assert(actor.has_value());
    assert(actor->type == ActorType::Agent);
}

void test_protocol_generation_reconnect_and_backend_isolation()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    const auto agent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(agent.has_value());

    BackendAgentConnectRequest incompatible;
    incompatible.backendId = "default";
    incompatible.agentInstanceId = "instance-incompatible";
    incompatible.protocolVersion = "vdr-suite-agent/9";
    incompatible.softwareVersion = "future/9";
    BackendAgentConnectResult rejected = fixture.service.connect(
        agentContext(*agent), incompatible, Now + 2);
    assert(!rejected.accepted);
    assert(rejected.reasonCode == "protocol_incompatible");
    assert(fixture.service.statusForBackend("default", Now + 2).state ==
        BackendAgentConnectionState::Incompatible);

    BackendAgentConnectResult first = connectAgent(fixture, *agent);
    assert(first.backendGeneration == 1);
    assert(first.disposition == "replace");

    BackendAgentConnectRequest resume;
    resume.backendId = "default";
    resume.agentInstanceId = "instance-1";
    resume.protocolVersion = "vdr-suite-agent/1";
    resume.softwareVersion = "vdr-suite-agent-test/1";
    resume.claimedBackendGeneration = 1;
    BackendAgentConnectResult resumed = fixture.service.connect(
        agentContext(*agent), resume, Now + 3);
    assert(resumed.accepted);
    assert(resumed.backendGeneration == 1);
    assert(resumed.disposition == "resume");

    resume.claimedHeartbeatSequence = 10;
    BackendAgentConnectResult resync = fixture.service.connect(
        agentContext(*agent), resume, Now + 4);
    assert(resync.accepted);
    assert(resync.disposition == "resync-required");

    BackendAgentConnectResult replaced = connectAgent(
        fixture, *agent, "instance-2");
    assert(replaced.backendGeneration == 2);
    assert(replaced.disposition == "replace");

    BackendAgentConnectRequest wrongBackend = resume;
    wrongBackend.backendId = "ferienhaus";
    BackendAgentConnectResult isolated = fixture.service.connect(
        agentContext(*agent), wrongBackend, Now + 5);
    assert(!isolated.accepted);
    assert(isolated.reasonCode == "agent_binding_mismatch");
}

void test_capabilities_heartbeat_lease_and_restart_persistence()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    auto agent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(agent.has_value());
    const BackendAgentConnectResult connected = connectAgent(fixture, *agent);

    BackendAgentCapabilityFacts facts;
    facts.adapters = {"suitebridge", "svdrp"};
    facts.observationDomains = {"backend-health", "recordings", "timers"};
    BackendAgentCapabilityResult capabilities = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, facts, Now + 3);
    assert(capabilities.accepted);
    assert(!capabilities.duplicate);
    BackendAgentCapabilityResult duplicateCapabilities = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, facts, Now + 4);
    assert(duplicateCapabilities.accepted);
    assert(duplicateCapabilities.duplicate);

    BackendAgentCapabilityResult gap = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        3, facts, Now + 4);
    assert(!gap.accepted);
    assert(gap.reasonCode == "capability_revision_gap");

    BackendAgentHeartbeatResult heartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, Now + 5);
    assert(heartbeat.accepted);
    assert(heartbeat.leaseExpiresAt == Now + 95);
    assert(!fixture.backendRegistryService.getBackend("default")->online);
    BackendAgentHeartbeatResult duplicateHeartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, Now + 6);
    assert(duplicateHeartbeat.accepted);
    assert(duplicateHeartbeat.duplicate);
    assert(duplicateHeartbeat.leaseExpiresAt == heartbeat.leaseExpiresAt);

    BackendAgentHeartbeatResult stale = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        0, Now + 7);
    assert(!stale.accepted);
    assert(stale.reasonCode == "stale_heartbeat_sequence");
    BackendAgentHeartbeatResult sequenceGap = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        3, Now + 7);
    assert(!sequenceGap.accepted);
    assert(sequenceGap.reasonCode == "heartbeat_sequence_gap");

    BackendAgentStatus online = fixture.service.statusForBackend("default", Now + 50);
    assert(online.state == BackendAgentConnectionState::Online);
    assert(online.capabilities.readOnly);
    assert(online.capabilities.adapters.size() == 2);
    BackendAgentStatus staleStatus = fixture.service.statusForBackend("default", Now + 100);
    assert(staleStatus.state == BackendAgentConnectionState::Stale);
    BackendAgentStatus offline = fixture.service.statusForBackend("default", Now + 126);
    assert(offline.state == BackendAgentConnectionState::Offline);
    assert(!fixture.backendRegistryService.getBackend("default")->online);

    BackendAgentRepository reopenedRepository(fixture.database);
    BackendAgentLifecycleService reopenedService(
        fixture.database,
        reopenedRepository,
        fixture.backendRegistryService,
        fixture.provisioningRepository,
        fixture.identityRepository,
        fixture.verifierRepository,
        fixture.accountabilityRepository);
    BackendAgentStatus restored = reopenedService.statusForBackend("default", Now + 50);
    assert(restored.state == BackendAgentConnectionState::Online);
    assert(restored.backendGeneration == connected.backendGeneration);
    assert(restored.heartbeatSequence == 1);
    assert(restored.capabilityRevision == 1);

    bool leaseResult = false;
    bool capabilityResult = false;
    for (const AccountabilityEvent& event : fixture.accountabilityRepository.listAll())
    {
        leaseResult = leaseResult || event.eventType == "agent.lease.result";
        capabilityResult = capabilityResult ||
            event.eventType == "agent.capabilities.result";
    }
    assert(leaseResult && capabilityResult);
}


void test_backend_health_observation_ingestion_and_atomicity()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    auto agent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(agent.has_value());
    const BackendAgentConnectResult connected = connectAgent(fixture, *agent);

    BackendAgentCapabilityFacts facts;
    facts.observationDomains = {"backend-health"};
    BackendAgentCapabilityResult capabilities = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, facts, Now + 3);
    assert(capabilities.accepted);
    BackendAgentHeartbeatResult heartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, Now + 4);
    assert(heartbeat.accepted);

    BackendAgentObservationRequest baseline;
    baseline.protocolVersion = "vdr-suite-agent/1";
    baseline.backendId = "default";
    baseline.agentInstanceId = "instance-1";
    baseline.backendGeneration = connected.backendGeneration;
    baseline.observationDomain = "backend-health";
    baseline.snapshotGeneration = 1;
    baseline.producerSequence = 1;
    baseline.kind = "completeSnapshot";
    baseline.capturedAt = Now + 4;
    baseline.resourceRevision = "heartbeat-1";
    baseline.agentState = "online";
    baseline.observedHeartbeatSequence = 1;

    BackendAgentObservationRequest changeBeforeBaseline = baseline;
    changeBeforeBaseline.kind = "changeBatch";
    BackendAgentObservationResult missingBaseline = fixture.service.ingestObservation(
        agentContext(*agent), changeBeforeBaseline, Now + 5);
    assert(!missingBaseline.accepted);
    assert(missingBaseline.resyncRequired);
    assert(missingBaseline.reasonCode == "observation_baseline_required");
    assert(!fixture.service.observationCursorForBackend(
        "default", "backend-health").present);

    BackendAgentObservationResult accepted = fixture.service.ingestObservation(
        agentContext(*agent), baseline, Now + 5);
    assert(accepted.accepted);
    assert(!accepted.replayed);
    assert(accepted.reasonCode == "complete_snapshot_accepted");
    assert(accepted.lastAcceptedSequence == 1);

    BackendAgentObservationCursor cursor = fixture.service.observationCursorForBackend(
        "default", "backend-health");
    assert(cursor.present);
    assert(cursor.snapshotGeneration == 1);
    assert(cursor.producerSequence == 1);
    assert(cursor.backendGeneration == connected.backendGeneration);

    BackendAgentObservationResult replay = fixture.service.ingestObservation(
        agentContext(*agent), baseline, Now + 6);
    assert(replay.accepted);
    assert(replay.replayed);
    assert(replay.reasonCode == "observation_replayed");

    BackendAgentObservationRequest conflict = baseline;
    conflict.resourceRevision = "heartbeat-one-conflict";
    BackendAgentObservationResult conflicting = fixture.service.ingestObservation(
        agentContext(*agent), conflict, Now + 7);
    assert(!conflicting.accepted);
    assert(conflicting.reasonCode == "observation_replay_conflict");

    BackendAgentHeartbeatResult secondHeartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        2, Now + 8);
    assert(secondHeartbeat.accepted);
    BackendAgentObservationRequest change = baseline;
    change.kind = "changeBatch";
    change.producerSequence = 2;
    change.capturedAt = Now + 8;
    change.resourceRevision = "heartbeat-2";
    change.observedHeartbeatSequence = 2;
    BackendAgentObservationResult changed = fixture.service.ingestObservation(
        agentContext(*agent), change, Now + 9);
    assert(changed.accepted);
    assert(changed.reasonCode == "change_batch_accepted");
    assert(changed.lastAcceptedSequence == 2);

    BackendAgentHeartbeatResult thirdHeartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        3, Now + 10);
    assert(thirdHeartbeat.accepted);
    BackendAgentObservationRequest gap = change;
    gap.producerSequence = 4;
    gap.capturedAt = Now + 10;
    gap.resourceRevision = "heartbeat-3-gap";
    gap.observedHeartbeatSequence = 3;
    BackendAgentObservationResult gapResult = fixture.service.ingestObservation(
        agentContext(*agent), gap, Now + 11);
    assert(!gapResult.accepted);
    assert(gapResult.resyncRequired);
    assert(gapResult.reasonCode == "observation_resync_required");
    assert(fixture.service.observationCursorForBackend(
        "default", "backend-health").producerSequence == 2);

    BackendAgentObservationRequest replacement = baseline;
    replacement.snapshotGeneration = 2;
    replacement.capturedAt = Now + 10;
    replacement.resourceRevision = "heartbeat-3-resync";
    replacement.observedHeartbeatSequence = 3;
    BackendAgentObservationResult replaced = fixture.service.ingestObservation(
        agentContext(*agent), replacement, Now + 12);
    assert(replaced.accepted);
    assert(replaced.reasonCode == "complete_snapshot_accepted");
    cursor = fixture.service.observationCursorForBackend("default", "backend-health");
    assert(cursor.snapshotGeneration == 2);
    assert(cursor.producerSequence == 1);

    BackendAgentObservationRequest staleInstance = replacement;
    staleInstance.agentInstanceId = "obsolete-instance";
    BackendAgentObservationResult fenced = fixture.service.ingestObservation(
        agentContext(*agent), staleInstance, Now + 13);
    assert(!fenced.accepted);
    assert(fenced.reasonCode == "stale_agent_instance");

    BackendAgentObservationRequest staleGeneration = replacement;
    staleGeneration.backendGeneration = connected.backendGeneration + 1;
    BackendAgentObservationResult generationFenced = fixture.service.ingestObservation(
        agentContext(*agent), staleGeneration, Now + 13);
    assert(!generationFenced.accepted);
    assert(generationFenced.reasonCode == "stale_backend_generation");

    BackendAgentCapabilityFacts noObservationDomains;
    BackendAgentCapabilityResult domainsRemoved = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        2, noObservationDomains, Now + 13);
    assert(domainsRemoved.accepted);
    BackendAgentObservationResult undeclaredResult = fixture.service.ingestObservation(
        agentContext(*agent), replacement, Now + 13);
    assert(!undeclaredResult.accepted);
    assert(undeclaredResult.reasonCode == "undeclared_observation_domain");
    BackendAgentCapabilityResult domainsRestored = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        3, facts, Now + 13);
    assert(domainsRestored.accepted);

    BackendAgentObservationRequest invalidDomain = replacement;
    invalidDomain.observationDomain = "recordings";
    BackendAgentObservationResult invalidDomainResult = fixture.service.ingestObservation(
        agentContext(*agent), invalidDomain, Now + 13);
    assert(!invalidDomainResult.accepted);
    assert(invalidDomainResult.reasonCode == "invalid_observation_envelope");

    BackendAgentHeartbeatResult fourthHeartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        4, Now + 14);
    assert(fourthHeartbeat.accepted);
    assert(fixture.database.execute(
        "CREATE TRIGGER fail_observation_cursor BEFORE UPDATE ON "
        "backend_agent_observation_cursors BEGIN "
        "SELECT RAISE(ABORT, 'forced cursor failure'); END;"));
    const std::int64_t receiptCount = rowCount(
        fixture.database, "backend_agent_observation_receipts");
    BackendAgentObservationRequest atomicChange = replacement;
    atomicChange.kind = "changeBatch";
    atomicChange.producerSequence = 2;
    atomicChange.capturedAt = Now + 14;
    atomicChange.resourceRevision = "heartbeat-4";
    atomicChange.observedHeartbeatSequence = 4;
    BackendAgentObservationResult failed = fixture.service.ingestObservation(
        agentContext(*agent), atomicChange, Now + 15);
    assert(!failed.accepted);
    assert(failed.reasonCode == "observation_persistence_failed");
    assert(rowCount(fixture.database, "backend_agent_observation_receipts") == receiptCount);
    cursor = fixture.service.observationCursorForBackend("default", "backend-health");
    assert(cursor.snapshotGeneration == 2);
    assert(cursor.producerSequence == 1);
    assert(fixture.database.execute("DROP TRIGGER fail_observation_cursor;"));

    std::string revokeReason;
    assert(fixture.service.revoke(
        adminContext(), agent->agentId, "observation-revocation-test",
        Now + 16, revokeReason));
    BackendAgentObservationResult revoked = fixture.service.ingestObservation(
        agentContext(*agent), atomicChange, Now + 17);
    assert(!revoked.accepted);
    assert(revoked.reasonCode == "agent_revoked_or_unknown");
    assert(fixture.service.observationCursorForBackend(
        "default", "backend-health").producerSequence == 1);
}

BackendAgentChannelFact channelFact(
    const std::string& channelId,
    std::uint64_t channelNumber,
    const std::string& name)
{
    BackendAgentChannelFact fact;
    fact.channelId = channelId;
    fact.channelNumber = channelNumber;
    fact.name = name;
    fact.provider = "Provider";
    fact.groupName = "Group";
    fact.radio = false;
    fact.encrypted = false;
    fact.enabled = true;
    return fact;
}

void test_channel_observation_ingestion_and_agent_owned_facts()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    auto agent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(agent.has_value());
    const BackendAgentConnectResult connected = connectAgent(fixture, *agent);

    BackendAgentCapabilityFacts facts;
    facts.adapters = {"channels-conf"};
    facts.observationDomains = {"channels"};
    BackendAgentCapabilityResult capabilities = fixture.service.publishCapabilities(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, facts, Now + 3);
    assert(capabilities.accepted);
    BackendAgentHeartbeatResult heartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        1, Now + 4);
    assert(heartbeat.accepted);

    BackendAgentObservationRequest baseline;
    baseline.protocolVersion = "vdr-suite-agent/1";
    baseline.backendId = "default";
    baseline.agentInstanceId = "instance-1";
    baseline.backendGeneration = connected.backendGeneration;
    baseline.observationDomain = "channels";
    baseline.snapshotGeneration = 1;
    baseline.producerSequence = 1;
    baseline.kind = "completeSnapshot";
    baseline.capturedAt = Now + 4;
    baseline.resourceRevision = "channels-revision-one";
    baseline.observedHeartbeatSequence = 1;
    baseline.channels = {
        channelFact("S19.2E-1-100-10", 1, "One"),
        channelFact("S19.2E-1-100-20", 2, "Two")};

    BackendAgentObservationResult accepted = fixture.service.ingestObservation(
        agentContext(*agent), baseline, Now + 5);
    assert(accepted.accepted);
    assert(accepted.reasonCode == "complete_snapshot_accepted");
    auto stored = fixture.agentRepository.channelFactsForBackend("default");
    assert(stored.size() == 2);
    assert(stored[0].channelId == "S19.2E-1-100-10");
    assert(stored[1].name == "Two");

    BackendAgentObservationRequest reordered = baseline;
    std::reverse(reordered.channels.begin(), reordered.channels.end());
    BackendAgentObservationResult replay = fixture.service.ingestObservation(
        agentContext(*agent), reordered, Now + 6);
    assert(replay.accepted);
    assert(replay.replayed);

    heartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        2, Now + 7);
    assert(heartbeat.accepted);
    BackendAgentObservationRequest change = baseline;
    change.channels.clear();
    change.kind = "changeBatch";
    change.producerSequence = 2;
    change.capturedAt = Now + 7;
    change.resourceRevision = "channels-revision-two";
    change.observedHeartbeatSequence = 2;
    change.upserts = {channelFact("S19.2E-1-100-20", 22, "Two Renamed")};
    change.removedChannelIds = {"S19.2E-1-100-10"};
    BackendAgentObservationResult changed = fixture.service.ingestObservation(
        agentContext(*agent), change, Now + 8);
    assert(changed.accepted);
    assert(changed.reasonCode == "change_batch_accepted");
    stored = fixture.agentRepository.channelFactsForBackend("default");
    assert(stored.size() == 1);
    assert(stored[0].channelNumber == 22);
    assert(stored[0].name == "Two Renamed");

    heartbeat = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-1", connected.backendGeneration,
        3, Now + 9);
    assert(heartbeat.accepted);
    BackendAgentObservationRequest unknownRemoval = change;
    unknownRemoval.producerSequence = 3;
    unknownRemoval.capturedAt = Now + 9;
    unknownRemoval.resourceRevision = "channels-revision-three";
    unknownRemoval.observedHeartbeatSequence = 3;
    unknownRemoval.upserts.clear();
    unknownRemoval.removedChannelIds = {"S19.2E-1-100-999"};
    BackendAgentObservationResult rejected = fixture.service.ingestObservation(
        agentContext(*agent), unknownRemoval, Now + 10);
    assert(!rejected.accepted);
    assert(rejected.reasonCode == "unknown_channel_removal");
    const BackendAgentObservationCursor cursor =
        fixture.service.observationCursorForBackend("default", "channels");
    assert(cursor.producerSequence == 2);
    assert(fixture.agentRepository.channelFactsForBackend("default").size() == 1);

    BackendAgentObservationRequest gap = unknownRemoval;
    gap.producerSequence = 4;
    gap.removedChannelIds.clear();
    gap.upserts = {channelFact("S19.2E-1-100-30", 30, "Thirty")};
    BackendAgentObservationResult gapResult = fixture.service.ingestObservation(
        agentContext(*agent), gap, Now + 10);
    assert(!gapResult.accepted);
    assert(gapResult.resyncRequired);
    assert(gapResult.reasonCode == "observation_resync_required");
}

void test_generation_fencing_revocation_and_accountability()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    auto agent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(agent.has_value());
    const BackendAgentConnectResult first = connectAgent(fixture, *agent, "instance-old");
    const BackendAgentConnectResult second = connectAgent(fixture, *agent, "instance-new");
    assert(second.backendGeneration == first.backendGeneration + 1);

    BackendAgentHeartbeatResult fenced = fixture.service.heartbeat(
        agentContext(*agent), "default", "instance-old", first.backendGeneration,
        1, Now + 5);
    assert(!fenced.accepted);
    assert(fenced.reasonCode == "stale_agent_instance");

    std::string reason;
    assert(fixture.service.revoke(
        adminContext(), agent->agentId, "operator-requested", Now + 6, reason));
    assert(reason == "agent_revoked");
    assert(fixture.service.statusForBackend("default", Now + 7).state ==
        BackendAgentConnectionState::Revoked);

    BackendAgentConnectRequest request;
    request.backendId = "default";
    request.agentInstanceId = "instance-after-revoke";
    request.protocolVersion = "vdr-suite-agent/1";
    request.softwareVersion = "test/1";
    BackendAgentConnectResult revoked = fixture.service.connect(
        agentContext(*agent), request, Now + 7);
    assert(!revoked.accepted);
    assert(revoked.reasonCode == "agent_revoked_or_unknown");

    const auto events = fixture.accountabilityRepository.listAll();
    assert(!events.empty());
    bool enrollmentEvent = false;
    bool connectionEvent = false;
    bool revocationEvent = false;
    bool connectionResultEvent = false;
    for (const AccountabilityEvent& event : events)
    {
        enrollmentEvent = enrollmentEvent || event.eventType == "agent.enrollment.succeeded";
        connectionEvent = connectionEvent || event.eventType == "agent.connection.accepted";
        revocationEvent = revocationEvent || event.eventType == "agent.revoked";
        connectionResultEvent = connectionResultEvent ||
            event.eventType == "agent.connection.result";
        assert(event.reasonCode.find(EnrollmentToken) == std::string::npos);
        assert(event.reasonCode.find(AgentSecret) == std::string::npos);
    }
    assert(enrollmentEvent && connectionEvent && revocationEvent &&
        connectionResultEvent);
}

void test_credential_rotation_fencing_idempotency_and_lease_invalidation()
{
    Fixture fixture;
    const auto enrollment = createAndConsumeEnrollment(fixture);
    const auto initialAgent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(initialAgent.has_value());
    const BackendAgentConnectResult connected = connectAgent(
        fixture, *initialAgent, "rotation-instance");

    BackendAgentCredentialRotationResult rotated = fixture.service.rotateCredential(
        agentContext(*initialAgent),
        "default",
        "rotation-instance",
        connected.backendGeneration,
        "rotation-request-1",
        1,
        RotatedAgentSecret,
        Now + 3);
    assert(rotated.accepted);
    assert(!rotated.idempotent);
    assert(rotated.credentialGeneration == 2);

    const auto updatedAgent = fixture.agentRepository.findAgent(enrollment.agentId);
    assert(updatedAgent.has_value());
    assert(updatedAgent->credentialGeneration == 2);
    assert(updatedAgent->leaseExpiresAt == 0);
    const auto verifier = fixture.verifierRepository.findByLogin(enrollment.agentId);
    assert(verifier.has_value());
    assert(!backendAgentVerifySecret(AgentSecret, verifier->passwordHash));
    assert(backendAgentVerifySecret(RotatedAgentSecret, verifier->passwordHash));

    BackendAgentCredentialRotationResult repeated = fixture.service.rotateCredential(
        agentContext(*updatedAgent),
        "default",
        "rotation-instance",
        connected.backendGeneration,
        "rotation-request-1",
        1,
        RotatedAgentSecret,
        Now + 4);
    assert(repeated.accepted);
    assert(repeated.idempotent);
    assert(repeated.credentialGeneration == 2);

    BackendAgentCredentialRotationResult conflict = fixture.service.rotateCredential(
        agentContext(*updatedAgent),
        "default",
        "rotation-instance",
        connected.backendGeneration,
        "rotation-request-1",
        1,
        "conflicting-rotated-secret-material-000000000000000000000001",
        Now + 5);
    assert(!conflict.accepted);
    assert(conflict.reasonCode == "credential_rotation_conflict");

    BackendAgentCredentialRotationResult stale = fixture.service.rotateCredential(
        agentContext(*updatedAgent),
        "default",
        "rotation-instance",
        connected.backendGeneration,
        "rotation-request-2",
        1,
        "next-rotated-secret-material-0000000000000000000000000001",
        Now + 6);
    assert(!stale.accepted);
    assert(stale.reasonCode == "stale_credential_generation");

    BackendAgentCredentialRotationResult fenced = fixture.service.rotateCredential(
        agentContext(*updatedAgent),
        "default",
        "obsolete-instance",
        connected.backendGeneration,
        "rotation-request-3",
        2,
        "next-rotated-secret-material-0000000000000000000000000002",
        Now + 7);
    assert(!fenced.accepted);
    assert(fenced.reasonCode == "obsolete_agent_generation");

    bool rotationSucceededEvent = false;
    for (const AccountabilityEvent& event : fixture.accountabilityRepository.listAll())
    {
        rotationSucceededEvent = rotationSucceededEvent ||
            (event.eventType == "agent.credential.rotation.result" &&
             event.outcome == "succeeded");
        assert(event.reasonCode.find(RotatedAgentSecret) == std::string::npos);
    }
    assert(rotationSucceededEvent);
}

void test_revoked_agent_can_be_replaced_without_losing_history()
{
    Fixture fixture;
    const auto firstEnrollment = createAndConsumeEnrollment(fixture);
    const auto firstAgent = fixture.agentRepository.findAgent(firstEnrollment.agentId);
    assert(firstAgent.has_value());
    std::string reason;
    assert(fixture.service.revoke(
        adminContext(), firstAgent->agentId, "replacement-required", Now + 2, reason));

    BackendAgentEnrollmentMaterial replay = fixture.service.enroll(
        "enr_test_1", EnrollmentToken, AgentSecret, Now + 3);
    assert(!replay.accepted);
    assert(replay.reasonCode == "enrollment_agent_revoked");

    const std::string replacementToken =
        "replacement-enrollment-token-material-000000000000000000000001";
    const std::string replacementSecret =
        "replacement-agent-secret-material-00000000000000000000000001";
    assert(fixture.service.createEnrollment(
        adminContext(), "enr_replacement", "default",
        backendAgentHashSecret(replacementToken), Now + 600, Now + 3, reason));
    BackendAgentEnrollmentMaterial replacement = fixture.service.enroll(
        "enr_replacement", replacementToken, replacementSecret, Now + 4);
    assert(replacement.accepted);
    assert(replacement.agentId != firstAgent->agentId);

    const auto active = fixture.agentRepository.findAgentForBackend("default");
    assert(active.has_value());
    assert(active->agentId == replacement.agentId);
    assert(!active->revoked);
    const auto historical = fixture.agentRepository.findAgent(firstAgent->agentId);
    assert(historical.has_value());
    assert(historical->revoked);
    assert(rowCount(fixture.database, "backend_agents") == 2);
}

class FallbackServer : public IHttpServer
{
public:
    HttpServerResponse handleRequest(const HttpServerRequest&) const override
    {
        HttpServerResponse response;
        response.statusCode = 299;
        response.body = "fallback";
        return response;
    }
};

std::string base64(const std::string& value)
{
    static const char Alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    for (std::size_t offset = 0; offset < value.size(); offset += 3)
    {
        const std::size_t remaining = value.size() - offset;
        const unsigned int first = static_cast<unsigned char>(value[offset]);
        const unsigned int second = remaining > 1 ? static_cast<unsigned char>(value[offset + 1]) : 0;
        const unsigned int third = remaining > 2 ? static_cast<unsigned char>(value[offset + 2]) : 0;
        const unsigned int encoded = (first << 16) | (second << 8) | third;
        output.push_back(Alphabet[(encoded >> 18) & 0x3f]);
        output.push_back(Alphabet[(encoded >> 12) & 0x3f]);
        output.push_back(remaining > 1 ? Alphabet[(encoded >> 6) & 0x3f] : '=');
        output.push_back(remaining > 2 ? Alphabet[encoded & 0x3f] : '=');
    }
    return output;
}

void test_http_protocol_and_redaction()
{
    Fixture fixture;
    const std::int64_t realNow = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    const std::string hash = backendAgentHashSecret(EnrollmentToken);
    std::string reason;
    assert(fixture.service.createEnrollment(
        adminContext(), "enr_http", "default", hash,
        realNow + 600, realNow, reason));
    BackendAgentHttpServer server(
        std::make_unique<FallbackServer>(),
        fixture.service,
        fixture.commandService,
        fixture.agentRepository,
        fixture.verifierRepository,
        fixture.identityRepository);

    HttpServerRequest enrollment;
    enrollment.method = "POST";
    enrollment.path = "/api/agent/v1/enroll";
    enrollment.headers["Authorization"] =
        "VDR-Suite-Enrollment enr_http:" + EnrollmentToken;
    enrollment.body = "{\"credentialSecret\":\"" + AgentSecret + "\"}";
    const HttpServerResponse enrolled = server.handleRequest(enrollment);
    assert(enrolled.statusCode == 200);
    assert(enrolled.body.find(AgentSecret) == std::string::npos);
    assert(enrolled.body.find(EnrollmentToken) == std::string::npos);

    const std::size_t agentStart = enrolled.body.find("\"agentId\":\"") + 11;
    const std::size_t agentEnd = enrolled.body.find('"', agentStart);
    const std::string agentId = enrolled.body.substr(agentStart, agentEnd - agentStart);
    assert(!agentId.empty());

    HttpServerRequest connect;
    connect.method = "POST";
    connect.path = "/api/agent/v1/connect";
    connect.headers["Authorization"] = "Basic " + base64(agentId + ":" + AgentSecret);
    connect.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"softwareVersion\":\"http-test/1\","
        "\"backendGeneration\":0,\"heartbeatSequence\":0,"
        "\"capabilityRevision\":0}";
    const HttpServerResponse connected = server.handleRequest(connect);
    assert(connected.statusCode == 200);
    assert(connected.body.find("\"backendGeneration\":1") != std::string::npos);
    assert(connected.body.find(AgentSecret) == std::string::npos);

    HttpServerRequest rotation;
    rotation.method = "POST";
    rotation.path = "/api/agent/v1/credentials/rotate";
    rotation.headers["Authorization"] =
        "Basic " + base64(agentId + ":" + AgentSecret);
    rotation.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"rotationId\":\"http-rotation-1\","
        "\"expectedCredentialGeneration\":1,\"credentialSecret\":\"" +
        RotatedAgentSecret + "\"}";
    const HttpServerResponse rotationResponse = server.handleRequest(rotation);
    assert(rotationResponse.statusCode == 200);
    assert(rotationResponse.body.find("\"credentialGeneration\":2") !=
        std::string::npos);
    assert(rotationResponse.body.find(RotatedAgentSecret) == std::string::npos);

    const HttpServerResponse oldCredentialDenied = server.handleRequest(connect);
    assert(oldCredentialDenied.statusCode == 401);
    connect.headers["Authorization"] =
        "Basic " + base64(agentId + ":" + RotatedAgentSecret);
    const HttpServerResponse rotatedCredentialAccepted = server.handleRequest(connect);
    assert(rotatedCredentialAccepted.statusCode == 200);
    assert(rotatedCredentialAccepted.body.find("\"credentialGeneration\":2") !=
        std::string::npos);

    HttpServerRequest capabilities;
    capabilities.method = "POST";
    capabilities.path = "/api/agent/v1/capabilities";
    capabilities.headers = connect.headers;
    capabilities.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"capabilityRevision\":1,"
        "\"readOnly\":true,\"adapters\":[\"channels-conf\"],"
        "\"observationDomains\":[\"backend-health\",\"channels\"]}";
    assert(server.handleRequest(capabilities).statusCode == 200);

    HttpServerRequest heartbeat;
    heartbeat.method = "POST";
    heartbeat.path = "/api/agent/v1/heartbeat";
    heartbeat.headers = connect.headers;
    heartbeat.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"heartbeatSequence\":1}";
    assert(server.handleRequest(heartbeat).statusCode == 200);

    HttpServerRequest observation;
    observation.method = "POST";
    observation.path = "/api/agent/v1/observations/backend-health";
    observation.headers = connect.headers;
    observation.body =
        "{\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"observationDomain\":\"backend-health\","
        "\"snapshotGeneration\":1,\"producerSequence\":1,"
        "\"kind\":\"completeSnapshot\",\"capturedAt\":" +
        std::to_string(realNow) +
        ",\"resourceRevision\":\"heartbeat-1\","
        "\"agentState\":\"online\",\"observedHeartbeatSequence\":1}";
    const HttpServerResponse observed = server.handleRequest(observation);
    assert(observed.statusCode == 200);
    assert(observed.body.find("\"outcome\":\"accepted\"") != std::string::npos);
    assert(observed.body.find(AgentSecret) == std::string::npos);
    assert(observed.body.find(RotatedAgentSecret) == std::string::npos);

    HttpServerRequest channelObservation;
    channelObservation.method = "POST";
    channelObservation.path = "/api/agent/v1/observations/channels";
    channelObservation.headers = connect.headers;
    channelObservation.body =
        "{\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"observationDomain\":\"channels\","
        "\"snapshotGeneration\":1,\"producerSequence\":1,"
        "\"kind\":\"completeSnapshot\",\"capturedAt\":" +
        std::to_string(realNow) +
        ",\"resourceRevision\":\"channels-http-one\","
        "\"observedHeartbeatSequence\":1,\"payload\":{\"channels\":[{"
        "\"channelId\":\"S19.2E-1-100-10\",\"channelNumber\":1,"
        "\"name\":\"One\",\"provider\":\"Provider\","
        "\"groupName\":\"Group\",\"radio\":false,"
        "\"encrypted\":false,\"enabled\":true}]}}";
    const HttpServerResponse channelObserved = server.handleRequest(channelObservation);
    assert(channelObserved.statusCode == 200);
    assert(channelObserved.body.find("\"outcome\":\"accepted\"") !=
        std::string::npos);
    assert(fixture.agentRepository.channelFactsForBackend("default").size() == 1);

    HttpServerRequest malformedChannel = channelObservation;
    malformedChannel.body = channelObservation.body.substr(
        0, channelObservation.body.size() - 1) + ",\"unknown\":true}";
    assert(server.handleRequest(malformedChannel).statusCode == 400);
    HttpServerRequest oversizedChannel = channelObservation;
    oversizedChannel.body.assign(512U * 1024U + 1U, 'x');
    assert(server.handleRequest(oversizedChannel).statusCode == 413);

    HttpServerRequest malformedObservation = observation;
    malformedObservation.body = "{\"protocolVersion\":\"vdr-suite-agent/1\"}";
    assert(server.handleRequest(malformedObservation).statusCode == 400);

    HttpServerRequest overflowObservation = observation;
    overflowObservation.body =
        "{\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"backendGeneration\":1,\"observationDomain\":\"backend-health\","
        "\"snapshotGeneration\":9223372036854775808,\"producerSequence\":1,"
        "\"kind\":\"completeSnapshot\",\"capturedAt\":" +
        std::to_string(realNow) +
        ",\"resourceRevision\":\"heartbeat-1\","
        "\"agentState\":\"online\",\"observedHeartbeatSequence\":1}";
    assert(server.handleRequest(overflowObservation).statusCode == 400);

    HttpServerRequest oversizedObservation = observation;
    oversizedObservation.body.assign(17U * 1024U, 'x');
    assert(server.handleRequest(oversizedObservation).statusCode == 413);

    HttpServerRequest malformedNumber = connect;
    malformedNumber.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"softwareVersion\":\"http-test/1\","
        "\"backendGeneration\":1x,\"heartbeatSequence\":0,"
        "\"capabilityRevision\":0}";
    assert(server.handleRequest(malformedNumber).statusCode == 400);

    HttpServerRequest overflowingNumber = connect;
    overflowingNumber.body =
        "{\"backendId\":\"default\",\"agentInstanceId\":\"http-instance\","
        "\"protocolVersion\":\"vdr-suite-agent/1\","
        "\"softwareVersion\":\"http-test/1\","
        "\"backendGeneration\":9223372036854775808,"
        "\"heartbeatSequence\":0,\"capabilityRevision\":0}";
    assert(server.handleRequest(overflowingNumber).statusCode == 400);

    HttpServerRequest invalid = connect;
    invalid.headers["Authorization"] = "Basic " + base64(agentId + ":wrong-secret-material-0000000000000000000000000000000");
    const HttpServerResponse denied = server.handleRequest(invalid);
    assert(denied.statusCode == 401);
    assert(denied.body.find("wrong-secret") == std::string::npos);

    HttpServerRequest oversized = connect;
    oversized.body.assign(16U * 1024U + 1U, 'x');
    const HttpServerResponse tooLarge = server.handleRequest(oversized);
    assert(tooLarge.statusCode == 413);

    HttpServerRequest ordinary;
    ordinary.method = "GET";
    ordinary.path = "/api/backends";
    assert(server.handleRequest(ordinary).statusCode == 299);
}

void test_accountability_failure_is_fail_closed()
{
    const std::string path = "/tmp/vdr-suite-agent-accountability-fail.db";
    std::remove(path.c_str());
    Database database;
    assert(database.open(path));
    BackendRegistry registry;
    BackendNode backend;
    backend.backendId = "default";
    registry.addBackend(backend);
    BackendRegistryService registryService(registry);
    SecurityIdentityRepository identities(database);
    SecurityIdentityProvisioningRepository provisioning(database);
    CredentialVerifierRepository verifiers(database);
    BackendAgentRepository agents(database);
    AccountabilityEventRepository missingAccountabilitySchema(database);
    assert(identities.ensureSchema());
    assert(verifiers.ensureSchema());
    assert(agents.ensureSchema());
    BackendAgentLifecycleService service(
        database, agents, registryService, provisioning, identities,
        verifiers, missingAccountabilitySchema);
    std::ostringstream expectedFailure;
    std::streambuf* originalError = std::cerr.rdbuf(expectedFailure.rdbuf());
    std::string reason;
    assert(!service.createEnrollment(
        adminContext(), "enr_fail_closed", "default",
        backendAgentHashSecret(EnrollmentToken), Now + 600, Now, reason));
    std::cerr.rdbuf(originalError);
    assert(reason == "accountability_unavailable");
    assert(!agents.findEnrollment("enr_fail_closed").has_value());
    assert(expectedFailure.str().find(EnrollmentToken) == std::string::npos);
    assert(expectedFailure.str().find(AgentSecret) == std::string::npos);
    database.close();
    std::remove(path.c_str());
}

void test_enrollment_transaction_rolls_back_partial_identity()
{
    Fixture fixture;
    const std::string hash = backendAgentHashSecret(EnrollmentToken);
    std::string reason;
    assert(fixture.service.createEnrollment(
        adminContext(), "enr_atomic", "default", hash,
        Now + 600, Now, reason));
    assert(fixture.database.execute(
        "CREATE TRIGGER fail_agent_verifier BEFORE INSERT ON "
        "security_basic_credential_verifiers BEGIN "
        "SELECT RAISE(ABORT, 'forced verifier failure'); END;"));

    const std::int64_t actorCount = rowCount(fixture.database, "security_actors");
    const std::int64_t deviceCount = rowCount(fixture.database, "security_devices");
    const std::int64_t credentialCount = rowCount(fixture.database, "security_credentials");
    BackendAgentEnrollmentMaterial failed = fixture.service.enroll(
        "enr_atomic", EnrollmentToken, AgentSecret, Now + 1);
    assert(!failed.accepted);
    assert(failed.reasonCode == "enrollment_persistence_failed");
    assert(rowCount(fixture.database, "security_actors") == actorCount);
    assert(rowCount(fixture.database, "security_devices") == deviceCount);
    assert(rowCount(fixture.database, "security_credentials") == credentialCount);
    assert(rowCount(fixture.database, "security_basic_credential_verifiers") == 0);
    assert(rowCount(fixture.database, "backend_agents") == 0);
    const auto enrollment = fixture.agentRepository.findEnrollment("enr_atomic");
    assert(enrollment.has_value());
    assert(enrollment->status == "pending");
}
}

int main()
{
    test_domain_values_and_capability_validation();
    test_enrollment_authorization_idempotency_and_binding();
    test_protocol_generation_reconnect_and_backend_isolation();
    test_capabilities_heartbeat_lease_and_restart_persistence();
    test_backend_health_observation_ingestion_and_atomicity();
    test_channel_observation_ingestion_and_agent_owned_facts();
    test_generation_fencing_revocation_and_accountability();
    test_credential_rotation_fencing_idempotency_and_lease_invalidation();
    test_revoked_agent_can_be_replaced_without_losing_history();
    test_http_protocol_and_redaction();
    test_accountability_failure_is_fail_closed();
    test_enrollment_transaction_rolls_back_partial_identity();
    std::cout << "test_backend_agent_lifecycle passed" << std::endl;
    return 0;
}
