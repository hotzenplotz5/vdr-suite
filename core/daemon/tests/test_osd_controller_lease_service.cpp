#include "LegacyOsdApiRuntime.h"
#include "LegacyOsdSessionService.h"
#include "OsdControllerLeaseService.h"
#include "OsdViewerBindingService.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

namespace
{
using SourceState = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;

std::int64_t nowValue = 1000;
std::uint64_t generation = 31;
std::uint64_t frameSequence = 1;
std::string surfaceId = "primary-native-osd";
std::string osdEpoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
bool viewAuthorized = true;
bool controlAuthorized = true;
bool adminRole = false;
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
    if (viewAuthorized)
        context.grants.push_back(PermissionGrant{"osd.view", backend});
    if (controlAuthorized)
        context.grants.push_back(PermissionGrant{"osd.control", backend});
    if (adminRole)
        context.grants.push_back(PermissionGrant{"role.admin", backend});
    context.permissionGrantResolution =
        PermissionGrantResolutionState::Resolved;
    return context;
}

BackendAgentStatus statusFor(
    const std::string& backend,
    std::int64_t)
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
    frame.frameSequence = frameSequence;
    frame.observedAt = static_cast<std::uint64_t>(nowValue);
    frame.fullFrame = true;
    frame.complete = true;
    frame.title = "PRIVATE_CONTROLLER_FRAME_TEXT";
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
    ++viewerCounter;
    return "ovb_controller_" + std::to_string(viewerCounter);
}

std::string nextLeaseId()
{
    ++leaseCounter;
    return "ocl_controller_" + std::to_string(leaseCounter);
}

struct Fixture
{
    LegacyOsdSessionService sessions;
    OsdViewerBindingService viewers;
    OsdControllerLeaseService controllers;
    LegacyOsdSession session;
    OsdViewerBinding first;
    OsdViewerBinding second;

    Fixture()
        : sessions(
              [](const std::string& actor, const std::string& backend)
                  -> std::optional<RequestSecurityContext>
              {
                  return contextFor(actor, backend);
              },
              statusFor,
              readFor,
              [] { return std::string("los_controller_001"); },
              [] { return nowValue; }),
          viewers(
              sessions,
              nextViewerId,
              [] { return nowValue; }),
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
              [] { return nowValue; })
    {
        LegacyOsdSessionCreateRequest create;
        create.actorId = "user:controller";
        create.clientInstanceId = "device:browser";
        create.backendId = "default";
        create.correlationId = "corr:controller";
        const auto created = sessions.create(create);
        assert(created.accepted);
        session = created.session;

        OsdViewerAttachRequest attach;
        attach.actorId = create.actorId;
        attach.clientInstanceId = create.clientInstanceId;
        attach.backendId = create.backendId;
        attach.legacyOsdSessionId = session.legacyOsdSessionId;
        const auto firstResult = viewers.attach(attach);
        assert(firstResult.accepted);
        first = firstResult.binding;
        const auto secondResult = viewers.attach(attach);
        assert(secondResult.accepted);
        second = secondResult.binding;
    }

    OsdControllerAcquireRequest acquireFor(
        const OsdViewerBinding& viewer) const
    {
        OsdControllerAcquireRequest request;
        request.actorId = viewer.actorId;
        request.clientInstanceId = viewer.clientInstanceId;
        request.backendId = viewer.backendId;
        request.legacyOsdSessionId = viewer.legacyOsdSessionId;
        request.viewerBindingId = viewer.viewerBindingId;
        return request;
    }

    OsdControllerStatusRequest statusForViewer(
        const OsdViewerBinding& viewer) const
    {
        OsdControllerStatusRequest request;
        request.actorId = viewer.actorId;
        request.clientInstanceId = viewer.clientInstanceId;
        request.backendId = viewer.backendId;
        request.legacyOsdSessionId = viewer.legacyOsdSessionId;
        request.viewerBindingId = viewer.viewerBindingId;
        return request;
    }
};

void resetGlobals()
{
    nowValue = 1000;
    generation = 31;
    frameSequence = 1;
    surfaceId = "primary-native-osd";
    osdEpoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    viewAuthorized = true;
    controlAuthorized = true;
    adminRole = false;
    backendWritable = true;
    viewerCounter = 0;
    leaseCounter = 0;
}

void testAcquireConflictRenewRelease()
{
    resetGlobals();
    Fixture fixture;

    const auto acquired = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(acquired.accepted);
    assert(acquired.hasLease);
    assert(acquired.lease.controllerLeaseEpoch == 1);
    assert(acquired.lease.leaseRevision == 1);
    assert(acquired.lease.state == OsdControllerLeaseState::Active);
    assert(acquired.lease.backendGeneration == generation);
    assert(acquired.lease.osdSurfaceId == surfaceId);
    assert(acquired.lease.osdEpoch == osdEpoch);
    assert(acquired.lease.grantedAt == nowValue);
    assert(acquired.lease.expiresAt ==
        nowValue + OsdControllerLeaseService::LeaseLifetimeSeconds);
    assert(acquired.lease.renewAfter ==
        nowValue + OsdControllerLeaseService::RenewAfterSeconds);

    const auto conflict = fixture.controllers.acquire(
        fixture.acquireFor(fixture.second));
    assert(!conflict.accepted);
    assert(conflict.error == "controller_lease_conflict");

    OsdControllerRenewRequest renew;
    renew.actorId = acquired.lease.actorId;
    renew.clientInstanceId = acquired.lease.clientInstanceId;
    renew.backendId = acquired.lease.backendId;
    renew.legacyOsdSessionId = acquired.lease.legacyOsdSessionId;
    renew.viewerBindingId = acquired.lease.viewerBindingId;
    renew.controllerLeaseId = acquired.lease.controllerLeaseId;
    renew.controllerLeaseEpoch = acquired.lease.controllerLeaseEpoch;
    renew.leaseRevision = acquired.lease.leaseRevision;

    const auto tooEarly = fixture.controllers.renew(renew);
    assert(!tooEarly.accepted);
    assert(tooEarly.error == "legacy_osd_controller_renew_too_early");

    nowValue = acquired.lease.renewAfter;
    const auto renewed = fixture.controllers.renew(renew);
    assert(renewed.accepted);
    assert(renewed.lease.controllerLeaseEpoch ==
        acquired.lease.controllerLeaseEpoch);
    assert(renewed.lease.leaseRevision ==
        acquired.lease.leaseRevision + 1);
    assert(renewed.lease.lastHeartbeatAt == nowValue);

    const auto staleRevision = fixture.controllers.renew(renew);
    assert(!staleRevision.accepted);
    assert(staleRevision.error == "revision_conflict");

    OsdControllerRenewRequest staleEpoch = renew;
    staleEpoch.leaseRevision = renewed.lease.leaseRevision;
    staleEpoch.controllerLeaseEpoch =
        renewed.lease.controllerLeaseEpoch + 1;
    const auto staleEpochResult = fixture.controllers.renew(staleEpoch);
    assert(!staleEpochResult.accepted);
    assert(staleEpochResult.error == "controller_lease_conflict");

    OsdControllerReleaseRequest release;
    release.actorId = renewed.lease.actorId;
    release.clientInstanceId = renewed.lease.clientInstanceId;
    release.backendId = renewed.lease.backendId;
    release.legacyOsdSessionId = renewed.lease.legacyOsdSessionId;
    release.viewerBindingId = renewed.lease.viewerBindingId;
    release.controllerLeaseId = renewed.lease.controllerLeaseId;
    release.controllerLeaseEpoch = renewed.lease.controllerLeaseEpoch;

    const auto released = fixture.controllers.release(release);
    assert(released.accepted);
    assert(released.lease.state == OsdControllerLeaseState::Released);
    const auto releasedAgain = fixture.controllers.release(release);
    assert(releasedAgain.accepted);
    assert(releasedAgain.lease.state == OsdControllerLeaseState::Released);

    const auto next = fixture.controllers.acquire(
        fixture.acquireFor(fixture.second));
    assert(next.accepted);
    assert(next.lease.controllerLeaseEpoch >
        acquired.lease.controllerLeaseEpoch);
}

void testExpiryAndExplicitRevocation()
{
    resetGlobals();
    Fixture fixture;
    const auto acquired = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(acquired.accepted);

    nowValue = acquired.lease.expiresAt;
    const auto afterExpiry = fixture.controllers.current(
        fixture.statusForViewer(fixture.second));
    assert(afterExpiry.accepted);
    assert(!afterExpiry.hasLease);

    const auto replacement = fixture.controllers.acquire(
        fixture.acquireFor(fixture.second));
    assert(replacement.accepted);

    const auto revoked = fixture.controllers.revoke(
        replacement.lease.controllerLeaseId,
        "administrative_revoke");
    assert(revoked.accepted);
    assert(revoked.lease.state == OsdControllerLeaseState::Revoked);
    assert(revoked.lease.revocationReason == "administrative_revoke");

    const auto afterRevoke = fixture.controllers.current(
        fixture.statusForViewer(fixture.first));
    assert(afterRevoke.accepted);
    assert(!afterRevoke.hasLease);
}

void testPermissionAndReadOnlyFencing()
{
    resetGlobals();
    Fixture fixture;

    controlAuthorized = false;
    const auto noGrant = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(!noGrant.accepted);
    assert(noGrant.error == "legacy_osd_control_not_authorized");

    adminRole = true;
    const auto adminOnly = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(!adminOnly.accepted);
    assert(adminOnly.error == "legacy_osd_control_not_authorized");

    controlAuthorized = true;
    adminRole = false;
    backendWritable = false;
    const auto readOnly = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(!readOnly.accepted);
    assert(readOnly.error == "read_only_backend");

    backendWritable = true;
    const auto acquired = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(acquired.accepted);

    controlAuthorized = false;
    const auto revoked = fixture.controllers.current(
        fixture.statusForViewer(fixture.second));
    assert(revoked.accepted);
    assert(!revoked.hasLease);
    const auto stored =
        fixture.controllers.find(acquired.lease.controllerLeaseId);
    assert(stored.has_value());
    assert(stored->state == OsdControllerLeaseState::Revoked);
    assert(stored->revocationReason ==
        "legacy_osd_control_not_authorized");
}

void testIdentityAndViewerFencing()
{
    resetGlobals();
    Fixture fixture;

    auto wrongActor = fixture.acquireFor(fixture.first);
    wrongActor.actorId = "user:other";
    assert(!fixture.controllers.acquire(wrongActor).accepted);

    auto wrongClient = fixture.acquireFor(fixture.first);
    wrongClient.clientInstanceId = "device:other";
    assert(!fixture.controllers.acquire(wrongClient).accepted);

    auto wrongBackend = fixture.acquireFor(fixture.first);
    wrongBackend.backendId = "other";
    assert(!fixture.controllers.acquire(wrongBackend).accepted);

    auto wrongSession = fixture.acquireFor(fixture.first);
    wrongSession.legacyOsdSessionId = "los_other";
    assert(!fixture.controllers.acquire(wrongSession).accepted);

    auto wrongViewer = fixture.acquireFor(fixture.first);
    wrongViewer.viewerBindingId = "ovb_other";
    assert(!fixture.controllers.acquire(wrongViewer).accepted);

    const auto acquired = fixture.controllers.acquire(
        fixture.acquireFor(fixture.first));
    assert(acquired.accepted);

    OsdViewerDetachRequest detach;
    detach.actorId = fixture.first.actorId;
    detach.clientInstanceId = fixture.first.clientInstanceId;
    detach.backendId = fixture.first.backendId;
    detach.legacyOsdSessionId = fixture.first.legacyOsdSessionId;
    detach.viewerBindingId = fixture.first.viewerBindingId;
    assert(fixture.viewers.detach(detach).accepted);

    const auto current = fixture.controllers.current(
        fixture.statusForViewer(fixture.second));
    assert(current.accepted);
    assert(!current.hasLease);
    const auto stored =
        fixture.controllers.find(acquired.lease.controllerLeaseId);
    assert(stored.has_value());
    assert(stored->state == OsdControllerLeaseState::Revoked);
}

void testGenerationSurfaceEpochAndSessionFencing()
{
    {
        resetGlobals();
        Fixture fixture;
        const auto acquired = fixture.controllers.acquire(
            fixture.acquireFor(fixture.first));
        assert(acquired.accepted);
        generation += 1;
        const auto current = fixture.controllers.current(
            fixture.statusForViewer(fixture.second));
        assert(!current.accepted ||
            !current.hasLease);
        const auto stored =
            fixture.controllers.find(acquired.lease.controllerLeaseId);
        assert(stored.has_value());
        assert(stored->state == OsdControllerLeaseState::Revoked);
    }

    {
        resetGlobals();
        Fixture fixture;
        const auto acquired = fixture.controllers.acquire(
            fixture.acquireFor(fixture.first));
        assert(acquired.accepted);
        surfaceId = "replacement-native-osd";
        const auto current = fixture.controllers.current(
            fixture.statusForViewer(fixture.second));
        assert(!current.accepted || !current.hasLease);
        const auto stored =
            fixture.controllers.find(acquired.lease.controllerLeaseId);
        assert(stored.has_value());
        assert(stored->state == OsdControllerLeaseState::Revoked);
    }

    {
        resetGlobals();
        Fixture fixture;
        const auto acquired = fixture.controllers.acquire(
            fixture.acquireFor(fixture.first));
        assert(acquired.accepted);
        osdEpoch = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
        const auto current = fixture.controllers.current(
            fixture.statusForViewer(fixture.second));
        assert(!current.accepted || !current.hasLease);
        const auto stored =
            fixture.controllers.find(acquired.lease.controllerLeaseId);
        assert(stored.has_value());
        assert(stored->state == OsdControllerLeaseState::Revoked);
    }

    {
        resetGlobals();
        Fixture fixture;
        const auto acquired = fixture.controllers.acquire(
            fixture.acquireFor(fixture.first));
        assert(acquired.accepted);
        nowValue = fixture.session.expiresAt;
        const auto current = fixture.controllers.current(
            fixture.statusForViewer(fixture.second));
        assert(!current.accepted || !current.hasLease);
        const auto stored =
            fixture.controllers.find(acquired.lease.controllerLeaseId);
        assert(stored.has_value());
        assert(stored->state == OsdControllerLeaseState::Revoked ||
            stored->state == OsdControllerLeaseState::Expired);
    }
}

void testApiLifecycleMetadataOnly()
{
    resetGlobals();
    Fixture fixture;
    auto& api = LegacyOsdApiRuntime::instance();
    api.reset();
    assert(api.configure(
        fixture.sessions,
        fixture.viewers,
        fixture.controllers));

    ApiResponse response;
    const std::string acquireBody =
        "{\"backendId\":\"default\","
        "\"legacyOsdSessionId\":\"los_controller_001\","
        "\"viewerBindingId\":\"ovb_controller_1\"}";
    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/controller-leases",
        acquireBody,
        "user:controller",
        "device:browser",
        "corr:api-controller",
        response));
    assert(response.statusCode == 201);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"controllerLeaseId\":\"ocl_controller_1\"")
        != std::string::npos);
    assert(response.body.find("\"controllerLeaseEpoch\":1")
        != std::string::npos);
    assert(response.body.find("PRIVATE_CONTROLLER_FRAME_TEXT")
        == std::string::npos);
    assert(response.body.find("\"actorId\"") == std::string::npos);
    assert(response.body.find("\"clientInstanceId\"") == std::string::npos);

    assert(api.tryHandleGet(
        "/api/vdr/legacy-osd/controller-leases/status"
        "?backend=default"
        "&session=los_controller_001"
        "&viewer=ovb_controller_2",
        response,
        "user:controller",
        "device:browser"));
    assert(response.statusCode == 200);
    assert(response.body.find("\"active\":true") != std::string::npos);

    nowValue += OsdControllerLeaseService::RenewAfterSeconds;
    const std::string renewBody =
        "{\"backendId\":\"default\","
        "\"legacyOsdSessionId\":\"los_controller_001\","
        "\"viewerBindingId\":\"ovb_controller_1\","
        "\"controllerLeaseId\":\"ocl_controller_1\","
        "\"controllerLeaseEpoch\":1,"
        "\"leaseRevision\":1}";
    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/controller-leases/renew",
        renewBody,
        "user:controller",
        "device:browser",
        "corr:api-controller",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"leaseRevision\":2")
        != std::string::npos);

    const std::string releaseBody =
        "{\"backendId\":\"default\","
        "\"legacyOsdSessionId\":\"los_controller_001\","
        "\"viewerBindingId\":\"ovb_controller_1\","
        "\"controllerLeaseId\":\"ocl_controller_1\","
        "\"controllerLeaseEpoch\":1}";
    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/controller-leases/release",
        releaseBody,
        "user:controller",
        "device:browser",
        "corr:api-controller",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"state\":\"released\"")
        != std::string::npos);

    assert(api.tryHandleGet(
        "/api/vdr/legacy-osd/controller-leases/status"
        "?backend=default"
        "&session=los_controller_001"
        "&viewer=ovb_controller_2",
        response,
        "user:controller",
        "device:browser"));
    assert(response.statusCode == 200);
    assert(response.body == "{\"active\":false}");
    api.reset();
}
}

int main()
{
    testAcquireConflictRenewRelease();
    testExpiryAndExplicitRevocation();
    testPermissionAndReadOnlyFencing();
    testIdentityAndViewerFencing();
    testGenerationSurfaceEpochAndSessionFencing();
    testApiLifecycleMetadataOnly();
    return 0;
}
