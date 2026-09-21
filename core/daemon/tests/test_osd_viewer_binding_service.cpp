#include "LegacyOsdApiRuntime.h"
#include "LegacyOsdSessionService.h"
#include "OsdViewerBindingService.h"

#include <cassert>
#include <cstdint>
#include <optional>
#include <string>

namespace
{
using SourceState = vdrsuite::agent::SuiteBridgeOsdFrameSourceState;

std::int64_t nowValue = 5000;
std::uint64_t generation = 21;
std::uint64_t frameSequence = 1;
std::string epoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
bool authorized = true;
SourceState sourceState = SourceState::Current;
int viewerCounter = 0;

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
    read.reasonCode = sourceState == SourceState::Current
        ? "osd_current" : "osd_resync_required";
    read.available = sourceState == SourceState::Current;
    read.snapshot.state = sourceState;
    read.snapshot.buffer.hasFrame = true;
    read.snapshot.buffer.resyncRequired =
        sourceState == SourceState::ResyncRequired;
    read.snapshot.buffer.observed.sourceConsistent =
        sourceState == SourceState::Current;
    auto& frame = read.snapshot.buffer.observed.frame;
    frame.surface.backendId = backend;
    frame.surface.backendGeneration = expectedGeneration;
    frame.surface.surfaceId = "primary-native-osd";
    frame.surface.osdEpoch = epoch;
    frame.state = OsdSurfaceState::Active;
    frame.kind = OsdFrameKind::Menu;
    frame.frameSequence = frameSequence;
    frame.observedAt = static_cast<std::uint64_t>(nowValue);
    frame.fullFrame = true;
    frame.complete = sourceState == SourceState::Current;
    frame.title = "PRIVATE_VIEWER_FRAME_TEXT";
    frame.items = {OsdTextItem{"One", true}, OsdTextItem{"Two", true}};
    frame.selectedIndex = 0;
    return read;
}

LegacyOsdSessionService makeSessionService(const std::string& id)
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

LegacyOsdSessionCreateRequest sessionRequest()
{
    LegacyOsdSessionCreateRequest request;
    request.actorId = "user:viewer";
    request.clientInstanceId = "device:viewer";
    request.backendId = "default";
    request.correlationId = "corr:68e";
    return request;
}

OsdViewerAttachRequest attachRequest(const std::string& sessionId)
{
    OsdViewerAttachRequest request;
    request.actorId = "user:viewer";
    request.clientInstanceId = "device:viewer";
    request.backendId = "default";
    request.legacyOsdSessionId = sessionId;
    return request;
}

OsdViewerReadRequest readRequest(
    const std::string& sessionId,
    const std::string& viewerId,
    std::uint64_t ack)
{
    OsdViewerReadRequest request;
    request.actorId = "user:viewer";
    request.clientInstanceId = "device:viewer";
    request.backendId = "default";
    request.legacyOsdSessionId = sessionId;
    request.viewerBindingId = viewerId;
    request.acknowledgedFrameSequence = ack;
    return request;
}

void resetState()
{
    nowValue = 5000;
    generation = 21;
    frameSequence = 1;
    epoch = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    authorized = true;
    sourceState = SourceState::Current;
    viewerCounter = 0;
}

void testMultiViewerIsolationAndResync()
{
    resetState();
    auto sessions = makeSessionService("los_viewers_001");
    assert(sessions.create(sessionRequest()).accepted);

    OsdViewerBindingService viewers(
        sessions,
        [] {
            ++viewerCounter;
            return "ovb_test_00" + std::to_string(viewerCounter);
        },
        [] { return nowValue; });

    const auto first = viewers.attach(attachRequest("los_viewers_001"));
    const auto second = viewers.attach(attachRequest("los_viewers_001"));
    assert(first.accepted && second.accepted);
    assert(first.binding.viewerBindingId != second.binding.viewerBindingId);
    assert(first.binding.state == OsdViewerBindingState::ResyncRequired);
    assert(!first.binding.controlAuthorized);
    assert(first.binding.backendGeneration == generation);
    assert(first.binding.osdEpoch == epoch);

    auto firstFrame = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 0));
    auto secondFrame = viewers.read(readRequest(
        "los_viewers_001", second.binding.viewerBindingId, 0));
    assert(firstFrame.accepted && firstFrame.hasFrame);
    assert(secondFrame.accepted && secondFrame.hasFrame);
    assert(firstFrame.frame.frameSequence == 1);
    assert(secondFrame.frame.frameSequence == 1);

    auto firstAck = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 1));
    assert(firstAck.accepted);
    assert(firstAck.state == OsdViewerDeliveryState::NoChange);

    frameSequence = 2;
    auto firstNext = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 1));
    assert(firstNext.accepted && firstNext.hasFrame);
    assert(firstNext.frame.frameSequence == 2);

    frameSequence = 3;
    auto slowSecond = viewers.read(readRequest(
        "los_viewers_001", second.binding.viewerBindingId, 0));
    assert(slowSecond.accepted);
    assert(slowSecond.state == OsdViewerDeliveryState::ResyncRequired);
    assert(slowSecond.reasonCode ==
        "viewer_backpressure_resync_required");
    assert(slowSecond.binding.lastDeliveredFrameSequence == 0);

    auto firstContinues = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 2));
    assert(firstContinues.accepted && firstContinues.hasFrame);
    assert(firstContinues.frame.frameSequence == 3);

    auto secondResynced = viewers.read(readRequest(
        "los_viewers_001", second.binding.viewerBindingId, 0));
    assert(secondResynced.accepted && secondResynced.hasFrame);
    assert(secondResynced.frame.frameSequence == 3);

    epoch = "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
    frameSequence = 1;
    auto epochFence = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 3));
    assert(epochFence.accepted);
    assert(epochFence.state == OsdViewerDeliveryState::ResyncRequired);
    assert(epochFence.binding.osdEpoch == epoch);
    assert(epochFence.binding.lastAcknowledgedFrameSequence == 0);

    auto epochFrame = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 0));
    assert(epochFrame.accepted && epochFrame.hasFrame);
    assert(epochFrame.frame.frameSequence == 1);

    authorized = false;
    auto revoked = viewers.read(readRequest(
        "los_viewers_001", first.binding.viewerBindingId, 1));
    assert(!revoked.accepted);
    assert(revoked.error == "legacy_osd_view_not_authorized");
    assert(!viewers.find(first.binding.viewerBindingId).has_value());

    const auto isolatedSecond = viewers.find(second.binding.viewerBindingId);
    assert(isolatedSecond.has_value());
}

void testGenerationExpiryAndBounds()
{
    resetState();
    auto sessions = makeSessionService("los_viewers_002");
    const auto created = sessions.create(sessionRequest());
    assert(created.accepted);

    OsdViewerBindingService viewers(
        sessions,
        [] {
            ++viewerCounter;
            return "ovb_bound_00" + std::to_string(viewerCounter);
        },
        [] { return nowValue; });

    const auto viewer = viewers.attach(attachRequest("los_viewers_002"));
    assert(viewer.accepted);

    ++generation;
    auto generationFence = viewers.read(readRequest(
        "los_viewers_002", viewer.binding.viewerBindingId, 0));
    assert(!generationFence.accepted);
    assert(generationFence.error ==
        "legacy_osd_backend_generation_changed");
    assert(!viewers.find(viewer.binding.viewerBindingId).has_value());

    generation = 21;
    auto boundedSessions = makeSessionService("los_viewers_003");
    assert(boundedSessions.create(sessionRequest()).accepted);
    viewerCounter = 0;
    OsdViewerBindingService bounded(
        boundedSessions,
        [] {
            ++viewerCounter;
            return "ovb_limit_00" + std::to_string(viewerCounter);
        },
        [] { return nowValue; });
    for (std::size_t index = 0;
         index < OsdViewerBindingService::MaximumBindingsPerSession;
         ++index)
        assert(bounded.attach(attachRequest("los_viewers_003")).accepted);
    const auto capacity =
        bounded.attach(attachRequest("los_viewers_003"));
    assert(!capacity.accepted);
    assert(capacity.error ==
        "legacy_osd_viewer_session_capacity_reached");

    auto expiringSessions = makeSessionService("los_viewers_004");
    nowValue = 7000;
    const auto expiring = expiringSessions.create(sessionRequest());
    assert(expiring.accepted);
    OsdViewerBindingService expiringViewers(
        expiringSessions,
        [] { return "ovb_expiring_001"; },
        [] { return nowValue; });
    const auto expiringViewer =
        expiringViewers.attach(attachRequest("los_viewers_004"));
    assert(expiringViewer.accepted);
    nowValue = expiring.session.expiresAt;
    const auto expired = expiringViewers.read(readRequest(
        "los_viewers_004", expiringViewer.binding.viewerBindingId, 0));
    assert(!expired.accepted);
    assert(expired.error == "legacy_osd_viewer_not_found" ||
           expired.error == "legacy_osd_viewer_expired");
}

void testApiBindingLifecycle()
{
    resetState();
    nowValue = 9000;
    auto sessions = makeSessionService("los_viewers_api");
    assert(sessions.create(sessionRequest()).accepted);
    OsdViewerBindingService viewers(
        sessions,
        [] { return "ovb_api_001"; },
        [] { return nowValue; });

    auto& api = LegacyOsdApiRuntime::instance();
    api.reset();
    assert(api.configure(sessions, viewers));

    ApiResponse response;
    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/viewers",
        "{\"backendId\":\"default\","
        "\"legacyOsdSessionId\":\"los_viewers_api\"}",
        "user:viewer", "device:viewer", "corr:viewer", response));
    assert(response.statusCode == 201);
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("ovb_api_001") != std::string::npos);
    assert(response.body.find("\"control\":false") != std::string::npos);
    assert(response.body.find("PRIVATE_VIEWER_FRAME_TEXT") ==
        std::string::npos);

    assert(response.body.find("PRIVATE_VIEWER_FRAME_TEXT") ==
        std::string::npos);

    assert(api.tryHandlePost(
        "/api/vdr/legacy-osd/viewers/detach",
        "{\"backendId\":\"default\","
        "\"legacyOsdSessionId\":\"los_viewers_api\","
        "\"viewerBindingId\":\"ovb_api_001\"}",
        "user:viewer", "device:viewer", "corr:detach", response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"state\":\"closed\"") !=
        std::string::npos);

    api.reset();
}
}

int main()
{
    testMultiViewerIsolationAndResync();
    testGenerationExpiryAndBounds();
    testApiBindingLifecycle();
    return 0;
}
