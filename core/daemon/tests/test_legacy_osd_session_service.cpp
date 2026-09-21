#include "LegacyOsdApiRuntime.h"
#include "LegacyOsdSessionService.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

namespace
{
using SourceState = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;

std::int64_t nowValue = 1000;
std::uint64_t generation = 7;
bool authorized = true;
BackendAgentConnectionState connectionState =
    BackendAgentConnectionState::Online;
SourceState sourceState = SourceState::Current;
std::string sourceReason = "osd_current";

RequestSecurityContext viewerContext(
    const std::string& actor,
    const std::string& backend)
{
    RequestSecurityContext context;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = {actor, ActorType::User, actor, true};
    context.grants = {PermissionGrant{"osd.view", backend}};
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
    status.state = connectionState;
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
    read.reasonCode = sourceReason;
    read.available = sourceState == SourceState::Current;
    read.snapshot.state = sourceState;
    read.snapshot.buffer.hasFrame = true;
    read.snapshot.buffer.observed.sourceConsistent = true;
    read.snapshot.buffer.contentFingerprint = 1234;
    auto& frame = read.snapshot.buffer.observed.frame;
    frame.surface.backendId = backend;
    frame.surface.backendGeneration = expectedGeneration;
    frame.surface.surfaceId = "primary-native-osd";
    frame.surface.osdEpoch = "0123456789abcdef0123456789abcdef";
    frame.fullFrame = true;
    frame.complete = sourceState == SourceState::Current;
    frame.frameSequence = 55;
    frame.title = "PRIVATE_FRAME_TEXT";
    if (sourceState == SourceState::ResyncRequired)
        read.snapshot.buffer.resyncRequired = true;
    return read;
}

LegacyOsdSessionService makeService(const std::string& id)
{
    return LegacyOsdSessionService(
        [](const std::string& actor, const std::string& backend)
            -> std::optional<RequestSecurityContext>
        {
            if (!authorized) return std::nullopt;
            return viewerContext(actor, backend);
        },
        statusFor,
        readFor,
        [id] { return id; },
        [] { return nowValue; });
}

LegacyOsdSessionCreateRequest request()
{
    LegacyOsdSessionCreateRequest value;
    value.actorId = "user:1";
    value.clientInstanceId = "device:browser";
    value.backendId = "default";
    value.correlationId = "corr:68d";
    return value;
}

void testService()
{
    auto service = makeService("los_test_001");
    auto created = service.create(request());
    assert(created.accepted);
    assert(created.session.backendGeneration == 7);
    assert(created.session.mode == LegacyOsdSessionMode::ViewOnly);
    assert(created.session.state == LegacyOsdSessionState::Active);
    assert(created.session.expiresAt ==
        1000 + LegacyOsdSessionService::SessionLifetimeSeconds);
    assert(created.session.capabilitySnapshot.viewAvailable);
    assert(!created.session.capabilitySnapshot.controlAvailable);
    assert(created.session.policySnapshot.viewAuthorized);
    assert(!created.session.policySnapshot.controlAuthorized);

    auto current = service.status(
        "los_test_001", "user:1", "device:browser", "default");
    assert(current.accepted);
    assert(current.session.sessionRevision == 2);

    assert(!service.status(
        "los_test_001", "other-user", "device:browser", "default").accepted);

    sourceState = SourceState::ResyncRequired;
    sourceReason = "osd_resync_required";
    auto resync = service.status(
        "los_test_001", "user:1", "device:browser", "default");
    assert(resync.accepted);
    assert(resync.session.state ==
        LegacyOsdSessionState::ResyncRequired);

    sourceState = SourceState::Current;
    sourceReason = "osd_current";
    ++generation;
    auto fenced = service.status(
        "los_test_001", "user:1", "device:browser", "default");
    assert(!fenced.accepted);
    assert(fenced.error == "legacy_osd_backend_generation_changed");
    assert(fenced.session.state == LegacyOsdSessionState::Closed);

    generation = 7;
    auto service2 = makeService("los_test_002");
    assert(service2.create(request()).accepted);
    authorized = false;
    auto revoked = service2.status(
        "los_test_002", "user:1", "device:browser", "default");
    assert(!revoked.accepted);
    assert(revoked.error == "legacy_osd_view_not_authorized");
    assert(revoked.session.state == LegacyOsdSessionState::Closed);

    authorized = true;
    auto service3 = makeService("los_test_003");
    created = service3.create(request());
    assert(created.accepted);
    nowValue = created.session.expiresAt;
    auto expired = service3.status(
        "los_test_003", "user:1", "device:browser", "default");
    assert(!expired.accepted);
    assert(expired.error == "legacy_osd_session_expired");
    assert(expired.session.state == LegacyOsdSessionState::Expired);
}

void testApi()
{
    nowValue = 2000;
    generation = 11;
    authorized = true;
    sourceState = SourceState::Current;
    sourceReason = "osd_current";

    auto service = makeService("los_api_001");
    auto& api = LegacyOsdApiRuntime::instance();
    api.reset();
    assert(api.configure(service));

    ApiResponse response;
    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/sessions",
        "{\"backendId\":\"default\"}",
        "user:1", "device:browser", "corr:api", response));
    assert(response.statusCode == 201);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("los_api_001") != std::string::npos);
    assert(response.body.find("\"control\":false") != std::string::npos);
    assert(response.body.find("PRIVATE_FRAME_TEXT") == std::string::npos);
    assert(response.body.find("\"title\"") == std::string::npos);

    assert(api.tryHandleGet(
        "/api/vdr/legacy-osd/sessions/status"
        "?backend=default&session=los_api_001",
        response, "user:1", "device:browser"));
    assert(response.statusCode == 200);
    assert(response.body.find("\"state\":\"active\"") !=
        std::string::npos);
    assert(response.body.find("PRIVATE_FRAME_TEXT") == std::string::npos);

    assert(api.tryHandleGet(
        "/api/vdr/legacy-osd/sessions/status"
        "?backend=default&session=los_api_001",
        response, "other-user", "device:browser"));
    assert(response.statusCode == 404);
    api.reset();
}
}

int main()
{
    testService();
    testApi();
    return 0;
}
