#include "BackendAgentCommandDelivery.h"
#include "BackendAgentLifecycle.h"
#include "Database.h"
#include "LegacyOsdInputService.h"
#include "LegacyOsdSessionService.h"
#include "OsdControllerLeaseService.h"
#include "OsdViewerBindingService.h"

#include <cassert>
#include <cstdio>
#include <optional>
#include <string>

namespace
{
using SourceState = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;

std::int64_t nowValue = 1000;
std::uint64_t generation = 31;
std::string surfaceId = "primary-native-osd";
std::string osdEpoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
bool backendWritable = true;
int viewerCounter = 0;
int leaseCounter = 0;

RequestSecurityContext contextFor(
    const std::string& actor,
    const std::string& backend)
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = {actor, ActorType::User, actor, true};
    context.grants.push_back(PermissionGrant{"osd.view", backend});
    context.grants.push_back(PermissionGrant{"osd.control", backend});
    context.permissionGrantResolution =
        PermissionGrantResolutionState::Resolved;
    return context;
}

BackendAgentStatus statusFor(const std::string& backend, std::int64_t)
{
    BackendAgentStatus status;
    status.present = true;
    status.backendId = backend;
    status.backendGeneration = generation;
    status.capabilityRevision = 1;
    status.state = BackendAgentConnectionState::Online;
    status.capabilities.readOnly = true;
    status.capabilities.adapters = {"suitebridge"};
    status.capabilities.observationDomains = {"osd"};
    return status;
}

BackendAgentOsdReadResult readFor(
    const RequestSecurityContext&,
    const std::string& backend,
    std::uint64_t expectedGeneration,
    std::int64_t)
{
    BackendAgentOsdReadResult read;
    read.available = true;
    read.reasonCode = "osd_current";
    read.snapshot.state = SourceState::Current;
    read.snapshot.buffer.hasFrame = true;
    OsdFrame& frame = read.snapshot.buffer.observed.frame;
    frame.surface.backendId = backend;
    frame.surface.backendGeneration = expectedGeneration;
    frame.surface.surfaceId = surfaceId;
    frame.surface.osdEpoch = osdEpoch;
    frame.state = OsdSurfaceState::Active;
    frame.kind = OsdFrameKind::Menu;
    frame.frameSequence = 1;
    frame.observedAt = static_cast<std::uint64_t>(nowValue);
    frame.fullFrame = true;
    frame.complete = true;
    frame.title = "PRIVATE_INPUT_FRAME_TEXT";
    frame.items = {OsdTextItem{"One", true}};
    frame.selectedIndex = 0;
    return read;
}

BackendAccessDecision backendPolicy(const std::string& backend)
{
    BackendAccessDecision decision;
    decision.backendId = backend;
    decision.backendFound = true;
    decision.accessMode = backendWritable ? "read-write" : "read-only";
    decision.readOnly = !backendWritable;
    decision.allowed = backendWritable;
    decision.reason = backendWritable
        ? "backend write access allowed"
        : "backend is read-only";
    return decision;
}

std::string nextViewerId()
{
    return "ovb_input_" + std::to_string(++viewerCounter);
}

std::string nextLeaseId()
{
    return "ocl_input_" + std::to_string(++leaseCounter);
}

struct Fixture
{
    std::string path = "/tmp/vdr-suite-phase68g-input.db";
    Database database;
    BackendAgentRepository agents;
    BackendAgentCommandRepository commands;
    LegacyOsdSessionService sessions;
    OsdViewerBindingService viewers;
    OsdControllerLeaseService controllers;
    LegacyOsdInputService input;
    LegacyOsdSession session;
    OsdViewerBinding viewer;
    OsdControllerLease lease;

    Fixture()
        : agents(database),
          commands(database),
          sessions(
              [](const std::string& actor, const std::string& backend)
                  -> std::optional<RequestSecurityContext>
              {
                  return contextFor(actor, backend);
              },
              statusFor,
              readFor,
              [] { return std::string("los_input_001"); },
              [] { return nowValue; }),
          viewers(sessions, nextViewerId, [] { return nowValue; }),
          controllers(
              sessions,
              viewers,
              [](const std::string& actor, const std::string& backend)
                  -> std::optional<RequestSecurityContext>
              {
                  return contextFor(actor, backend);
              },
              backendPolicy,
              nextLeaseId,
              [] { return nowValue; }),
          input(
              sessions,
              viewers,
              controllers,
              agents,
              commands,
              [] { return nowValue; })
    {
        std::remove(path.c_str());
        assert(database.open(path));
        assert(agents.ensureSchema());
        assert(commands.ensureSchema());
        assert(database.execute(
            "INSERT INTO backend_agents("
            "agent_id,backend_id,actor_id,device_id,credential_id,"
            "credential_generation,agent_instance_id,backend_generation,"
            "protocol_version,software_version,heartbeat_sequence,"
            "capability_revision,last_connected_at,last_heartbeat_at,"
            "lease_expires_at,created_at,updated_at"
            ") VALUES("
            "'agt_input','default','actor_input_agent','dev_input_agent',"
            "'cred_input_agent',1,'inst_input',31,'vdr-suite-agent/1',"
            "'test',2,1,1000,1000,2000,1000,1000);"));
        assert(database.execute(
            "INSERT INTO backend_agent_command_capabilities("
            "backend_id,agent_id,agent_instance_id,backend_generation,"
            "command_type,published_at"
            ") VALUES("
            "'default','agt_input','inst_input',31,"
            "'vdr.legacy-osd.input',1000);"));

        LegacyOsdSessionCreateRequest create;
        create.actorId = "user:controller";
        create.clientInstanceId = "device:browser";
        create.backendId = "default";
        create.correlationId = "corr:input";
        const auto created = sessions.create(create);
        assert(created.accepted);

        OsdViewerAttachRequest attach;
        attach.actorId = create.actorId;
        attach.clientInstanceId = create.clientInstanceId;
        attach.backendId = create.backendId;
        attach.legacyOsdSessionId = created.session.legacyOsdSessionId;
        const auto attached = viewers.attach(attach);
        assert(attached.accepted);
        viewer = attached.binding;

        OsdControllerAcquireRequest acquire;
        acquire.actorId = viewer.actorId;
        acquire.clientInstanceId = viewer.clientInstanceId;
        acquire.backendId = viewer.backendId;
        acquire.legacyOsdSessionId = viewer.legacyOsdSessionId;
        acquire.viewerBindingId = viewer.viewerBindingId;
        const auto acquired = controllers.acquire(acquire);
        assert(acquired.accepted && acquired.hasLease);
        lease = acquired.lease;

        const auto currentSession = sessions.find(lease.legacyOsdSessionId);
        assert(currentSession.has_value());
        session = *currentSession;
    }

    ~Fixture()
    {
        database.close();
        std::remove(path.c_str());
    }

    LegacyOsdInputCommand command(
        const std::string& id,
        LegacyOsdInputAction action = LegacyOsdInputAction::Up) const
    {
        LegacyOsdInputCommand value;
        value.inputCommandId = id;
        value.legacyOsdSessionId = lease.legacyOsdSessionId;
        value.sessionRevision = session.sessionRevision;
        value.viewerBindingId = lease.viewerBindingId;
        value.controllerLeaseId = lease.controllerLeaseId;
        value.controllerLeaseEpoch = lease.controllerLeaseEpoch;
        value.leaseRevision = lease.leaseRevision;
        value.actorId = lease.actorId;
        value.clientInstanceId = lease.clientInstanceId;
        value.backendId = lease.backendId;
        value.backendGeneration = lease.backendGeneration;
        value.osdSurfaceId = lease.osdSurfaceId;
        value.osdEpoch = lease.osdEpoch;
        value.action = action;
        value.inputMode = "press";
        value.repeatCount = 1;
        value.deadline = nowValue + 2;
        value.correlationId = "corr:" + id;
        return value;
    }

    BackendAgentCommandReceipt receiptFor(
        const std::string& inputCommandId) const
    {
        const auto assignment = commands.findAssignmentForOperation(
            "default",
            "osdi:" + inputCommandId,
            kLegacyOsdInputCommandType);
        assert(assignment.has_value());
        BackendAgentCommandReceipt receipt;
        receipt.commandId = assignment->commandId;
        receipt.requestFingerprint = assignment->requestFingerprint;
        receipt.jobId = assignment->jobId;
        receipt.attemptId = assignment->attemptId;
        receipt.claimEpoch = assignment->claimEpoch;
        receipt.backendId = assignment->backendId;
        receipt.agentId = assignment->agentId;
        receipt.agentInstanceId = assignment->agentInstanceId;
        receipt.backendGeneration = assignment->backendGeneration;
        receipt.receiptCategory = "accepted";
        receipt.receivedAt = nowValue;
        receipt.reasonCode = "durably_recorded";
        return receipt;
    }
};

void resetGlobals()
{
    nowValue = 1000;
    generation = 31;
    surfaceId = "primary-native-osd";
    osdEpoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    backendWritable = true;
    viewerCounter = 0;
    leaseCounter = 0;
}

void testAdmissionDuplicateAndReceiptFence()
{
    resetGlobals();
    Fixture fixture;
    const auto command = fixture.command("input_001");
    const auto first = fixture.input.submit(command);
    assert(first.accepted);
    assert(!first.idempotent);
    assert(first.category == "accepted_for_dispatch");
    assert(!first.agentCommandId.empty());

    const auto duplicate = fixture.input.submit(command);
    assert(duplicate.accepted);
    assert(duplicate.idempotent);
    assert(duplicate.agentCommandId == first.agentCommandId);

    auto conflict = command;
    conflict.action = LegacyOsdInputAction::Down;
    const auto conflicting = fixture.input.submit(conflict);
    assert(!conflicting.accepted);
    assert(conflicting.error == "legacy_osd_input_duplicate_conflict");

    std::string reason;
    const auto receipt = fixture.receiptFor(command.inputCommandId);
    assert(fixture.input.revalidateReceipt(receipt, reason));
    assert(reason.empty());

    const auto revoked = fixture.controllers.revoke(
        fixture.lease.controllerLeaseId,
        "administrative_revoke");
    assert(revoked.accepted);
    assert(!fixture.input.revalidateReceipt(receipt, reason));
    assert(reason == "controller_lease_conflict");
}

void testEpochGenerationDeadlineAndReadOnlyFences()
{
    {
        resetGlobals();
        Fixture fixture;
        const auto command = fixture.command("input_epoch");
        assert(fixture.input.submit(command).accepted);
        const auto receipt = fixture.receiptFor(command.inputCommandId);
        osdEpoch = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        std::string reason;
        assert(!fixture.input.revalidateReceipt(receipt, reason));
        assert(reason == "controller_lease_conflict" ||
            reason == "legacy_osd_controller_epoch_changed");
    }

    {
        resetGlobals();
        Fixture fixture;
        const auto command = fixture.command("input_generation");
        assert(fixture.input.submit(command).accepted);
        const auto receipt = fixture.receiptFor(command.inputCommandId);
        generation += 1;
        std::string reason;
        assert(!fixture.input.revalidateReceipt(receipt, reason));
    }

    {
        resetGlobals();
        Fixture fixture;
        auto expired = fixture.command("input_expired");
        expired.deadline = nowValue;
        const auto result = fixture.input.submit(expired);
        assert(!result.accepted);
        assert(result.error == "legacy_osd_input_deadline_invalid");

        auto excessive = fixture.command("input_far");
        excessive.deadline =
            nowValue + LegacyOsdInputService::MaximumDeadlineLeadSeconds + 1;
        const auto far = fixture.input.submit(excessive);
        assert(!far.accepted);
        assert(far.error == "legacy_osd_input_deadline_invalid");
    }

    {
        resetGlobals();
        Fixture fixture;
        const auto command = fixture.command("input_readonly");
        backendWritable = false;
        const auto result = fixture.input.submit(command);
        assert(!result.accepted);
        assert(result.error == "read_only_backend" ||
            result.error == "controller_lease_conflict");
    }
}

void testBoundedRate()
{
    resetGlobals();
    Fixture fixture;
    for (std::uint64_t index = 0;
         index < LegacyOsdInputService::MaximumAcceptedCommandsPerSecond;
         ++index)
    {
        const auto result = fixture.input.submit(
            fixture.command("input_rate_" + std::to_string(index)));
        assert(result.accepted);
    }
    const auto limited = fixture.input.submit(
        fixture.command("input_rate_limited"));
    assert(!limited.accepted);
    assert(limited.error == "rate_limited");

    ++nowValue;
    const auto nextWindow = fixture.input.submit(
        fixture.command("input_rate_next"));
    assert(nextWindow.accepted);
}
}

int main()
{
    testAdmissionDuplicateAndReceiptFence();
    testEpochGenerationDeadlineAndReadOnlyFences();
    testBoundedRate();
    return 0;
}
