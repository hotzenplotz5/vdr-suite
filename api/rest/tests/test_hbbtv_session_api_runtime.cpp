#include "HbbtvApiRuntime.h"

#include "HbbtvApplicationSessionService.h"
#include "SuiteBridgeHbbtvPresentationResolver.h"
#include "SuiteBridgeHbbtvMediaResolver.h"

#include <cassert>
#include <string>

namespace
{

BroadcastApplicationRef applicationRef()
{
    BroadcastApplicationRef ref;
    ref.backendId = "default";
    ref.backendGeneration = 7;
    ref.channelId = "C-1-1051-10301";
    ref.provider.providerId = "vdr-plugin-web";
    ref.provider.providerSchemaVersion = 1;
    ref.provider.capabilityRevision = 17;
    ref.provider.observedAt = 12345;
    ref.applicationId = 1;
    ref.descriptorRevision = 17;
    return ref;
}

class FakeDiscovery final : public IHbbtvApplicationDiscoveryService
{
public:
    BroadcastApplicationDiscoverySnapshot discoverApplications(
        const std::string& backendId,
        const std::string& channelId) const override
    {
        BroadcastApplicationDiscoverySnapshot snapshot;
        snapshot.backendId = backendId;
        snapshot.backendGeneration = 7;
        snapshot.payloadValid = true;
        snapshot.receiverActive = true;
        snapshot.result = "ok";
        snapshot.channelId = channelId;
        snapshot.revision = 17;
        snapshot.observedAt = 12345;

        BroadcastApplicationDescriptor descriptor;
        descriptor.ref = applicationRef();
        descriptor.controlCode = 1;
        descriptor.priority = 2;
        descriptor.name = "HBBTV-Start";
        snapshot.applications.push_back(descriptor);
        return snapshot;
    }
};

class FakePresentation final : public IHbbtvPresentationSource
{
public:
    HbbtvPresentationFrame readPresentation(
        const std::string& sessionId,
        std::uint64_t knownRevision) override
    {
        lastSessionId = sessionId;
        lastKnownRevision = knownRevision;

        HbbtvPresentationFrame frame;
        frame.available = true;
        frame.frameRevision = 9;
        frame.observedAt = 1234;
        frame.renderWidth = 1280;
        frame.renderHeight = 720;
        if (knownRevision == 9)
        {
            frame.unchanged = true;
            return frame;
        }

        frame.qoi = "qoif-test-frame";
        return frame;
    }

    std::string lastSessionId;
    std::uint64_t lastKnownRevision = 0;
};

class FakeMedia final : public IHbbtvMediaSourceResolver
{
public:
    HbbtvMediaSource resolveMedia(
        const std::string& sessionId) override
    {
        lastSessionId = sessionId;
        HbbtvMediaSource media;
        media.available = true;
        media.state = HbbtvMediaSourceState::Streaming;
        media.mediaRevision = 3;
        media.fullscreen = false;
        media.x = 100;
        media.y = 50;
        media.width = 640;
        media.height = 360;
        media.unixSocketPath =
            "/run/vdr/vdr-suite-hbbtv-media/private.sock";
        return media;
    }

    std::string lastSessionId;
};

class FakeRuntime final : public IHbbtvRuntimeControl
{
public:
    SuiteBridgeHbbtvRuntimeResolution control(
        const SuiteBridgeHbbtvRuntimeRequest& request) override
    {
        last = request;

        SuiteBridgeHbbtvRuntimeResolution result;
        result.payloadValid = true;
        result.sessionId = request.sessionId;
        result.channelId = request.channelId;
        result.applicationId = request.applicationId;
        result.descriptorRevision = request.descriptorRevision;

        switch (request.operation)
        {
            case SuiteBridgeHbbtvRuntimeOperation::Launch:
                result.operation = "launch";
                result.result = "accepted";
                result.resultCode = 1;
                result.state = "starting";
                result.stateCode = 1;
                break;
            case SuiteBridgeHbbtvRuntimeOperation::Status:
                result.operation = "status";
                result.result = "ok";
                result.resultCode = 0;
                result.state = "active";
                result.stateCode = 2;
                break;
            case SuiteBridgeHbbtvRuntimeOperation::Input:
                result.operation = "input";
                result.result = "ok";
                result.resultCode = 0;
                result.state = "active";
                result.stateCode = 2;
                result.action = "LEFT";
                break;
            case SuiteBridgeHbbtvRuntimeOperation::Close:
                result.operation = "close";
                result.result = "accepted";
                result.resultCode = 1;
                result.state = "closing";
                result.stateCode = 3;
                break;
        }
        return result;
    }

    SuiteBridgeHbbtvRuntimeRequest last;
};

}

int main()
{
    FakeDiscovery discovery;
    FakeRuntime runtime;
    FakePresentation presentation;
    FakeMedia media;

    HbbtvApplicationSessionService sessions(
        discovery,
        [&runtime](const std::string&) -> IHbbtvRuntimeControl* {
            return &runtime;
        },
        [](const std::string&, const std::string&, const std::string&) {
            return true;
        },
        [] { return std::string("bas_api_test"); },
        [] { return static_cast<std::int64_t>(1000); });

    HbbtvApiRuntime& api = HbbtvApiRuntime::instance();
    api.reset();
    assert(api.configure(
        discovery,
        sessions,
        [&presentation](const std::string& backendId)
            -> IHbbtvPresentationSource* {
            return backendId == "default" ? &presentation : nullptr;
        },
        [&media](const std::string& backendId)
            -> IHbbtvMediaSourceResolver* {
            return backendId == "default" ? &media : nullptr;
        }));

    ApiResponse response;
    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions",
        "{\"backendId\":\"default\","
        "\"channelId\":\"C-1-1051-10301\","
        "\"applicationId\":1,"
        "\"descriptorRevision\":17}",
        "user-1",
        "device-1",
        "corr-1",
        response));
    assert(response.statusCode == 202);
    assert(response.body.find("\"sessionId\":\"bas_api_test\"") !=
        std::string::npos);
    assert(response.body.find("\"state\":\"starting\"") !=
        std::string::npos);
    assert(response.body.find("urlBase") == std::string::npos);
    assert(response.body.find("HBBRUN") == std::string::npos);
    assert(response.body.find("VK_") == std::string::npos);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions/status",
        "{\"backendId\":\"default\","
        "\"sessionId\":\"bas_api_test\"}",
        "user-1",
        "device-1",
        "corr-2",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"state\":\"active\"") !=
        std::string::npos);

    assert(api.tryHandleGet(
        "/api/vdr/broadcast/hbbtv/sessions/presentation"
        "?backend=default&session=bas_api_test&revision=0",
        response,
        "user-1",
        "device-1"));
    assert(response.statusCode == 200);
    assert(response.contentType == "image/qoi");
    assert(response.body == "qoif-test-frame");
    assert(response.headers.at("X-Vdr-Suite-Hbbtv-Revision") == "9");
    assert(response.headers.at("X-Vdr-Suite-Hbbtv-Width") == "1280");
    assert(response.headers.at("X-Vdr-Suite-Hbbtv-Height") == "720");
    assert(presentation.lastSessionId == "bas_api_test");
    assert(presentation.lastKnownRevision == 0);

    assert(api.tryHandleGet(
        "/api/vdr/broadcast/hbbtv/sessions/presentation"
        "?backend=default&session=bas_api_test&revision=9",
        response,
        "user-1",
        "device-1"));
    assert(response.statusCode == 204);
    assert(response.body.empty());
    assert(response.headers.at("X-Vdr-Suite-Hbbtv-Revision") == "9");
    assert(response.headers.find("X-Vdr-Suite-Hbbtv-Width") ==
        response.headers.end());
    assert(response.headers.find("X-Vdr-Suite-Hbbtv-Height") ==
        response.headers.end());
    assert(presentation.lastKnownRevision == 9);

    assert(api.tryHandleGet(
        "/api/vdr/broadcast/hbbtv/sessions/media"
        "?backend=default&session=bas_api_test",
        response,
        "user-1",
        "device-1"));
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json; charset=utf-8");
    assert(response.body.find("\"available\":true") != std::string::npos);
    assert(response.body.find("\"state\":\"streaming\"") != std::string::npos);
    assert(response.body.find("\"mediaRevision\":3") != std::string::npos);
    assert(response.body.find("\"fullscreen\":false") != std::string::npos);
    assert(response.body.find(
        "\"geometry\":{\"x\":100,\"y\":50,\"width\":640,\"height\":360}") !=
        std::string::npos);
    assert(response.body.find("socketPath") == std::string::npos);
    assert(response.body.find("/run/vdr/") == std::string::npos);
    assert(media.lastSessionId == "bas_api_test");

    assert(api.tryHandleGet(
        "/api/vdr/broadcast/hbbtv/sessions/media"
        "?backend=default&session=bas_api_test",
        response,
        "other-user",
        "device-1"));
    assert(response.statusCode == 403);

    assert(api.tryHandleGet(
        "/api/vdr/broadcast/hbbtv/sessions/presentation"
        "?backend=default&session=bas_api_test&revision=0",
        response,
        "other-user",
        "device-1"));
    assert(response.statusCode == 403);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions/input",
        "{\"backendId\":\"default\","
        "\"sessionId\":\"bas_api_test\","
        "\"action\":\"left\"}",
        "user-1",
        "device-1",
        "corr-3",
        response));
    assert(response.statusCode == 200);
    assert(runtime.last.inputAction == SuiteBridgeHbbtvInputAction::Left);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions/input",
        "{\"backendId\":\"default\","
        "\"sessionId\":\"bas_api_test\","
        "\"action\":\"VK_LEFT\"}",
        "user-1",
        "device-1",
        "corr-4",
        response));
    assert(response.statusCode == 400);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions/close",
        "{\"backendId\":\"default\","
        "\"sessionId\":\"bas_api_test\"}",
        "user-1",
        "device-1",
        "corr-5",
        response));
    assert(response.statusCode == 202);
    assert(response.body.find("\"state\":\"closing\"") !=
        std::string::npos);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions/status",
        "{\"backendId\":\"other\","
        "\"sessionId\":\"bas_api_test\"}",
        "user-1",
        "device-1",
        "corr-6",
        response));
    assert(response.statusCode == 409);
    assert(response.body.find("hbbtv_session_backend_mismatch") !=
        std::string::npos);

    assert(api.tryHandlePost(
        "/api/vdr/broadcast/hbbtv/sessions",
        "{\"backendId\":\"default\","
        "\"channelId\":\"C-1-1051-10301\","
        "\"applicationId\":1,"
        "\"descriptorRevision\":17}",
        "user-1",
        "bad/client",
        "corr-7",
        response));
    assert(response.statusCode == 403);
    assert(response.body.find("hbbtv_request_context_invalid") !=
        std::string::npos);

    api.reset();
    return 0;
}
