#include "HbbtvApplicationSessionService.h"

#include <algorithm>
#include <cassert>
#include <string>
#include <utility>
#include <vector>

namespace
{

BroadcastApplicationRef applicationRef(std::uint64_t generation = 7)
{
    BroadcastApplicationRef ref;
    ref.backendId = "default";
    ref.backendGeneration = generation;
    ref.channelId = "C-1-1051-10301";
    ref.provider.providerId = "vdr-plugin-web";
    ref.provider.providerSchemaVersion = 1;
    ref.provider.capabilityRevision = 17;
    ref.provider.observedAt = 12345;
    ref.applicationId = 1;
    ref.descriptorRevision = 17;
    return ref;
}

BroadcastApplicationDiscoverySnapshot discovery(
    const BroadcastApplicationRef& ref)
{
    BroadcastApplicationDiscoverySnapshot snapshot;
    snapshot.backendId = ref.backendId;
    snapshot.backendGeneration = ref.backendGeneration;
    snapshot.payloadValid = true;
    snapshot.receiverActive = true;
    snapshot.result = "ok";
    snapshot.channelId = ref.channelId;
    snapshot.revision = ref.descriptorRevision;
    snapshot.observedAt = ref.provider.observedAt;

    BroadcastApplicationDescriptor descriptor;
    descriptor.ref = ref;
    descriptor.controlCode = 1;
    descriptor.priority = 2;
    descriptor.name = "HBBTV-Start";
    snapshot.applications.push_back(descriptor);
    return snapshot;
}

class FakeDiscovery final : public IHbbtvApplicationDiscoveryService
{
public:
    BroadcastApplicationDiscoverySnapshot discoverApplications(
        const std::string& backendId,
        const std::string& channelId) const override
    {
        ++calls;
        lastBackend = backendId;
        lastChannel = channelId;
        return snapshot;
    }

    mutable int calls = 0;
    mutable std::string lastBackend;
    mutable std::string lastChannel;
    BroadcastApplicationDiscoverySnapshot snapshot;
};

class FakeRuntime final : public IHbbtvRuntimeControl
{
public:
    SuiteBridgeHbbtvRuntimeResolution control(
        const SuiteBridgeHbbtvRuntimeRequest& request) override
    {
        ++calls;
        lastRequest = request;
        if (replies.empty()) return {};
        const auto reply = replies.front();
        replies.erase(replies.begin());
        return reply;
    }

    int calls = 0;
    SuiteBridgeHbbtvRuntimeRequest lastRequest;
    std::vector<SuiteBridgeHbbtvRuntimeResolution> replies;
};

SuiteBridgeHbbtvRuntimeResolution runtimeReply(
    const SuiteBridgeHbbtvRuntimeRequest& request,
    const std::string& result,
    std::uint8_t resultCode,
    const std::string& state,
    std::uint8_t stateCode)
{
    SuiteBridgeHbbtvRuntimeResolution reply;
    reply.payloadValid = true;
    switch (request.operation)
    {
        case SuiteBridgeHbbtvRuntimeOperation::Launch:
            reply.operation = "launch";
            break;
        case SuiteBridgeHbbtvRuntimeOperation::Status:
            reply.operation = "status";
            break;
        case SuiteBridgeHbbtvRuntimeOperation::Input:
            reply.operation = "input";
            break;
        case SuiteBridgeHbbtvRuntimeOperation::Close:
            reply.operation = "close";
            break;
    }
    reply.result = result;
    reply.resultCode = resultCode;
    reply.state = state;
    reply.stateCode = stateCode;
    reply.sessionId = request.sessionId;
    reply.channelId = request.channelId;
    reply.applicationId = request.applicationId;
    reply.descriptorRevision = request.descriptorRevision;
    return reply;
}

BroadcastApplicationLaunchRequest launchRequest()
{
    BroadcastApplicationLaunchRequest request;
    request.actorId = "user-1";
    request.clientContext = "living-room-tv";
    request.correlationContext = "corr-1";
    request.application = applicationRef();
    request.lifetimeSeconds = 3600;
    return request;
}

}

int main()
{
    FakeDiscovery discoveryService;
    discoveryService.snapshot = discovery(applicationRef());

    FakeRuntime runtime;
    bool allowLaunch = true;
    bool allowInput = true;
    bool allowManage = true;
    std::int64_t now = 1000;

    HbbtvApplicationSessionService service(
        discoveryService,
        [&runtime](const std::string& backendId) -> IHbbtvRuntimeControl* {
            return backendId == "default" ? &runtime : nullptr;
        },
        [&](const std::string& permission,
            const std::string& actorId,
            const std::string& backendId) {
            assert(actorId == "user-1");
            assert(backendId == "default");
            if (permission == "broadcast.hbbtv.launch") return allowLaunch;
            if (permission == "broadcast.hbbtv.input") return allowInput;
            if (permission == "broadcast.session.manage_own") return allowManage;
            return false;
        },
        [] { return std::string("bas_001122"); },
        [&now] { return now; });

    SuiteBridgeHbbtvRuntimeRequest expectedLaunch;
    expectedLaunch.operation = SuiteBridgeHbbtvRuntimeOperation::Launch;
    expectedLaunch.sessionId = "bas_001122";
    expectedLaunch.channelId = "C-1-1051-10301";
    expectedLaunch.applicationId = 1;
    expectedLaunch.descriptorRevision = 17;
    runtime.replies.push_back(runtimeReply(
        expectedLaunch, "accepted", 1, "starting", 1));

    const auto launched = service.launch(launchRequest());
    assert(launched.accepted);
    assert(launched.error.empty());
    assert(launched.session.broadcastApplicationSessionId == "bas_001122");
    assert(launched.session.actorId == "user-1");
    assert(launched.session.clientContext == "living-room-tv");
    assert(launched.session.backendId == "default");
    assert(launched.session.backendGeneration == 7);
    assert(launched.session.applicationDescriptorRevision == 17);
    assert(launched.session.state == BroadcastApplicationSessionState::Starting);
    assert(launched.session.createdAt == 1000);
    assert(launched.session.expiresAt == 4600);
    assert(launched.session.correlationContext == "corr-1");
    assert(launched.session.runtimeCapabilityProfile.close);
    assert(std::find(
        launched.session.runtimeCapabilityProfile.inputActions.begin(),
        launched.session.runtimeCapabilityProfile.inputActions.end(),
        "left") !=
        launched.session.runtimeCapabilityProfile.inputActions.end());
    assert(discoveryService.calls == 1);
    assert(runtime.calls == 1);
    assert(runtime.lastRequest.sessionId == "bas_001122");

    SuiteBridgeHbbtvRuntimeRequest expectedStatus = expectedLaunch;
    expectedStatus.operation = SuiteBridgeHbbtvRuntimeOperation::Status;
    runtime.replies.push_back(runtimeReply(
        expectedStatus, "ok", 0, "active", 2));

    const auto active = service.refresh(
        "bas_001122", "user-1", "living-room-tv");
    assert(active.accepted);
    assert(active.session.state == BroadcastApplicationSessionState::Active);

    const auto presentationAccess = service.authorizePresentation(
        "bas_001122",
        "user-1",
        "living-room-tv");
    assert(presentationAccess.accepted);
    assert(presentationAccess.session.state ==
        BroadcastApplicationSessionState::Active);

    const auto presentationWrongOwner = service.authorizePresentation(
        "bas_001122",
        "user-1",
        "other-client");
    assert(!presentationWrongOwner.accepted);
    assert(presentationWrongOwner.error ==
        "hbbtv_session_owner_mismatch");

    SuiteBridgeHbbtvRuntimeRequest expectedInput = expectedLaunch;
    expectedInput.operation = SuiteBridgeHbbtvRuntimeOperation::Input;
    expectedInput.inputAction = SuiteBridgeHbbtvInputAction::Left;
    auto inputReply =
        runtimeReply(expectedInput, "ok", 0, "active", 2);
    inputReply.action = "LEFT";
    runtime.replies.push_back(inputReply);

    const auto input = service.input(
        "bas_001122",
        "user-1",
        "living-room-tv",
        SuiteBridgeHbbtvInputAction::Left);
    assert(input.accepted);
    assert(runtime.lastRequest.operation ==
        SuiteBridgeHbbtvRuntimeOperation::Input);
    assert(runtime.lastRequest.inputAction ==
        SuiteBridgeHbbtvInputAction::Left);

    const int callsBeforeOwnerMismatch = runtime.calls;
    const auto wrongOwner = service.input(
        "bas_001122",
        "other-user",
        "living-room-tv",
        SuiteBridgeHbbtvInputAction::Left);
    assert(!wrongOwner.accepted);
    assert(wrongOwner.error == "hbbtv_session_owner_mismatch");
    assert(runtime.calls == callsBeforeOwnerMismatch);

    BroadcastApplicationRef changed = applicationRef();
    changed.descriptorRevision = 18;
    changed.provider.capabilityRevision = 18;
    discoveryService.snapshot = discovery(changed);

    const int callsBeforeStale = runtime.calls;
    const auto staleInput = service.input(
        "bas_001122",
        "user-1",
        "living-room-tv",
        SuiteBridgeHbbtvInputAction::Right);
    assert(!staleInput.accepted);
    assert(staleInput.error == "hbbtv_application_context_stale");
    assert(staleInput.session.state ==
        BroadcastApplicationSessionState::Suspended);
    assert(runtime.calls == callsBeforeStale);

    SuiteBridgeHbbtvRuntimeRequest expectedClose = expectedLaunch;
    expectedClose.operation = SuiteBridgeHbbtvRuntimeOperation::Close;
    runtime.replies.push_back(runtimeReply(
        expectedClose, "accepted", 1, "closing", 3));

    const auto closing = service.close(
        "bas_001122",
        "user-1",
        "living-room-tv",
        "user_close");
    assert(closing.accepted);
    assert(closing.session.state ==
        BroadcastApplicationSessionState::Closing);
    assert(closing.session.closeReason == "user_close");

    runtime.replies.push_back(runtimeReply(
        expectedStatus, "session_not_active", 6, "none", 0));

    const auto closed = service.refresh(
        "bas_001122", "user-1", "living-room-tv");
    assert(closed.accepted);
    assert(closed.session.state == BroadcastApplicationSessionState::Closed);
    assert(closed.session.closeReason == "user_close");

    FakeDiscovery deniedDiscovery;
    deniedDiscovery.snapshot = discovery(applicationRef());
    FakeRuntime deniedRuntime;
    HbbtvApplicationSessionService deniedService(
        deniedDiscovery,
        [&deniedRuntime](const std::string&) -> IHbbtvRuntimeControl* {
            return &deniedRuntime;
        },
        [](const std::string&, const std::string&, const std::string&) {
            return false;
        },
        [] { return std::string("bas_denied"); },
        [] { return static_cast<std::int64_t>(1000); });

    const auto denied = deniedService.launch(launchRequest());
    assert(!denied.accepted);
    assert(denied.error == "hbbtv_launch_not_authorized");
    assert(deniedDiscovery.calls == 0);
    assert(deniedRuntime.calls == 0);

    FakeDiscovery staleDiscovery;
    staleDiscovery.snapshot = discovery(applicationRef(8));
    FakeRuntime staleRuntime;
    HbbtvApplicationSessionService staleService(
        staleDiscovery,
        [&staleRuntime](const std::string&) -> IHbbtvRuntimeControl* {
            return &staleRuntime;
        },
        [](const std::string&, const std::string&, const std::string&) {
            return true;
        },
        [] { return std::string("bas_stale"); },
        [] { return static_cast<std::int64_t>(1000); });

    const auto staleLaunch = staleService.launch(launchRequest());
    assert(!staleLaunch.accepted);
    assert(staleLaunch.error == "hbbtv_application_context_stale");
    assert(staleRuntime.calls == 0);

    FakeDiscovery expiringDiscovery;
    expiringDiscovery.snapshot = discovery(applicationRef());
    FakeRuntime expiringRuntime;
    std::int64_t expiringNow = 1000;
    HbbtvApplicationSessionService expiringService(
        expiringDiscovery,
        [&expiringRuntime](const std::string&) -> IHbbtvRuntimeControl* {
            return &expiringRuntime;
        },
        [](const std::string&, const std::string&, const std::string&) {
            return true;
        },
        [] { return std::string("bas_expiring"); },
        [&expiringNow] { return expiringNow; });

    BroadcastApplicationLaunchRequest shortLaunch = launchRequest();
    shortLaunch.lifetimeSeconds = 60;
    SuiteBridgeHbbtvRuntimeRequest expiringLaunch = expectedLaunch;
    expiringLaunch.sessionId = "bas_expiring";
    expiringRuntime.replies.push_back(runtimeReply(
        expiringLaunch, "accepted", 1, "starting", 1));
    const auto expiring = expiringService.launch(shortLaunch);
    assert(expiring.accepted);

    SuiteBridgeHbbtvRuntimeRequest expiringStatus = expiringLaunch;
    expiringStatus.operation = SuiteBridgeHbbtvRuntimeOperation::Status;
    expiringRuntime.replies.push_back(runtimeReply(
        expiringStatus, "ok", 0, "active", 2));
    assert(expiringService.refresh(
        "bas_expiring", "user-1", "living-room-tv").accepted);

    expiringNow = 1060;
    const int callsBeforeExpiry = expiringRuntime.calls;
    const auto expired = expiringService.input(
        "bas_expiring",
        "user-1",
        "living-room-tv",
        SuiteBridgeHbbtvInputAction::Ok);
    assert(!expired.accepted);
    assert(expired.error == "hbbtv_session_expired");
    assert(expired.session.state ==
        BroadcastApplicationSessionState::Expired);
    assert(expiringRuntime.calls == callsBeforeExpiry);

    return 0;
}
