#include "BackendRegistry.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "SuiteBridgeTeletextResolver.h"
#include "TeletextApiRuntime.h"
#include "TeletextControlPlaneReadService.h"
#include "VdrSnapshot.h"

#include <cassert>
#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{

class FakeTeletextTransport final : public ISuiteBridgeTeletextTransport
{
public:
    SuiteBridgeTeletextCommandReply discoverTeletext() override
    {
        ++discoverCalls;
        return capabilityReply;
    }

    SuiteBridgeTeletextCommandReply requestTeletextPage(
        const SuiteBridgeTeletextPageRequest& request) override
    {
        ++pageCalls;
        lastRequest = request;
        return pageReply;
    }

    SuiteBridgeTeletextCommandReply capabilityReply;
    SuiteBridgeTeletextCommandReply pageReply;
    SuiteBridgeTeletextPageRequest lastRequest;
    int discoverCalls = 0;
    int pageCalls = 0;
};

class FakeBackendAuthority final : public ITeletextBackendAuthority
{
public:
    TeletextBackendAuthorityState stateForBackend(
        const std::string&,
        std::int64_t) const override
    {
        return state;
    }

    TeletextBackendAuthorityState state;
};

SuiteBridgeTeletextCommandReply capabilityReply(bool available = true)
{
    SuiteBridgeTeletextCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload =
        std::string("{\"schemaVersion\":1,\"provider\":\"osdteletext\",") +
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.teletext.page\","
        "\"available\":" + (available ? "true" : "false") +
        ",\"rows\":25,\"columns\":40,"
        "\"subpages\":true,\"level1\":true,\"x26Partial\":true,"
        "\"conceal\":true,\"blink\":true,\"doubleSize\":true}";
    return reply;
}

SuiteBridgeTeletextCommandReply pageReply()
{
    std::ostringstream json;
    json
        << "{\"schemaVersion\":1,\"result\":\"ok\","
        << "\"channel\":\"C-1-1051-10301\","
        << "\"requestedPage\":100,\"source\":\"live\","
        << "\"receiverActive\":true,\"teletextAvailable\":true,"
        << "\"serviceEpoch\":9,\"revision\":17,\"observedAt\":12345,"
        << "\"page\":100,\"subpage\":0,\"complete\":false,"
        << "\"rows\":25,\"columns\":40,\"text\":[";

    for (int row = 0; row < 25; ++row)
    {
        if (row != 0) json << ',';
        json << (row == 0
            ? "\"Börse und Nachrichten\""
            : "\"Teletext row\"");
    }

    json << "],\"cells\":[";
    for (int cell = 0; cell < 1000; ++cell)
    {
        if (cell != 0) json << ',';
        json << "[32,32,0,7,0,0,0]";
    }
    json << "]}";

    SuiteBridgeTeletextCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload = json.str();
    return reply;
}

VdrSnapshot makeSnapshot()
{
    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    snapshot.status.enabled = true;
    snapshot.status.state = "connected";

    VdrChannel channel;
    channel.id = "C-1-1051-10301";
    channel.name = "Das Erste HD";
    snapshot.channels.push_back(channel);
    return snapshot;
}

struct Fixture
{
    Fixture()
        : registryService(registry),
          cacheService(cache),
          accessService(cacheService),
          snapshotReadService(accessService),
          resolver(transport),
          service(
              registryService,
              snapshotReadService,
              authority,
              [this](const std::string& backendId)
                  -> SuiteBridgeTeletextResolver* {
                  return backendId == "default" ? &resolver : nullptr;
              },
              []() -> std::int64_t { return 4242; })
    {
        BackendNode backend;
        backend.backendId = "default";
        backend.enabled = true;
        registry.addBackend(backend);

        cache.updateForBackend("default", makeSnapshot());

        transport.capabilityReply = capabilityReply();
        transport.pageReply = pageReply();
        authority.state.present = true;
        authority.state.online = true;
        authority.state.backendGeneration = 7;

        TeletextApiRuntime::instance().reset();
        assert(TeletextApiRuntime::instance().configure(service));
    }

    ~Fixture()
    {
        TeletextApiRuntime::instance().reset();
    }

    BackendRegistry registry;
    BackendRegistryService registryService;
    SnapshotCache cache;
    SnapshotCacheService cacheService;
    SnapshotAccessService accessService;
    VdrSnapshotReadService snapshotReadService;
    FakeTeletextTransport transport;
    SuiteBridgeTeletextResolver resolver;
    FakeBackendAuthority authority;
    TeletextControlPlaneReadService service;
};

void testServiceRouteSerializesNormalizedIdentity()
{
    Fixture fixture;
    ApiResponse response;

    const bool handled = TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/service?backend=default&channel=C-1-1051-10301",
        response);

    assert(handled);
    assert(response.statusCode == 200);
    assert(response.contentType == "application/json; charset=utf-8");
    assert(response.headers.at("Cache-Control") == "no-store");
    assert(response.body.find("\"backendGeneration\":7") != std::string::npos);
    assert(response.body.find("\"provider\":{\"id\":\"osdteletext\"") != std::string::npos);
    assert(response.body.find("\"channelId\":\"C-1-1051-10301\"") != std::string::npos);
    assert(response.body.find("\"rows\":25") != std::string::npos);
    assert(fixture.transport.discoverCalls == 1);
}

void testPageRouteReturnsUtf8AndProviderEvidence()
{
    Fixture fixture;
    ApiResponse response;

    const bool handled = TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=100&subpage=auto",
        response);

    assert(handled);
    assert(response.statusCode == 200);
    assert(response.body.find("\"pageAvailable\":true") != std::string::npos);
    assert(response.body.find("\"generation\":9") != std::string::npos);
    assert(response.body.find("\"observedAt\":12345") != std::string::npos);
    assert(response.body.find("\"revision\":17") != std::string::npos);
    assert(response.body.find("Börse und Nachrichten") != std::string::npos);
    assert(response.body.find("[32,32,0,7,0,0,0]") != std::string::npos);
    assert(fixture.transport.discoverCalls == 1);
    assert(fixture.transport.pageCalls == 1);
    assert(fixture.transport.lastRequest.pageNumber == 100);
    assert(fixture.transport.lastRequest.automaticSubpage);
}

void testExplicitSubpageIsForwarded()
{
    Fixture fixture;
    fixture.transport.pageReply = pageReply();
    ApiResponse response;

    const bool handled = TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=100&subpage=42",
        response);

    assert(handled);
    assert(response.statusCode == 200);
    assert(fixture.transport.pageCalls == 1);
    assert(!fixture.transport.lastRequest.automaticSubpage);
    assert(fixture.transport.lastRequest.subpageCode == 42);
}

void testMalformedRequestsFailBeforeProviderRead()
{
    Fixture fixture;
    const std::vector<std::string> invalid = {
        "/api/vdr/broadcast/teletext/service?backend=default",
        "/api/vdr/broadcast/teletext/service?backend=default&channel=C-1-1051-10301&extra=x",
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=99",
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=900",
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=100&subpage=65535",
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C%20bad&page=100"
    };

    for (const std::string& target : invalid)
    {
        ApiResponse response;
        assert(TeletextApiRuntime::instance().tryHandleGet(target, response));
        assert(response.statusCode == 400);
    }

    assert(fixture.transport.discoverCalls == 0);
    assert(fixture.transport.pageCalls == 0);
}

void testUnavailableServiceIsTruthful()
{
    Fixture fixture;
    fixture.transport.capabilityReply = capabilityReply(false);

    ApiResponse serviceResponse;
    assert(TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/service?backend=default&channel=C-1-1051-10301",
        serviceResponse));
    assert(serviceResponse.statusCode == 200);
    assert(serviceResponse.body.find("\"available\":false") != std::string::npos);

    ApiResponse pageResponse;
    assert(TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/page?backend=default&channel=C-1-1051-10301&page=100",
        pageResponse));
    assert(pageResponse.statusCode == 409);
    assert(pageResponse.body.find("teletext_service_unavailable") != std::string::npos);
    assert(fixture.transport.pageCalls == 0);
}

void testUnrelatedRouteIsNotClaimed()
{
    Fixture fixture;
    ApiResponse response;
    response.statusCode = 777;

    assert(!TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/channels",
        response));
    assert(response.statusCode == 777);
}

void testUnconfiguredRuntimeFailsClosed()
{
    TeletextApiRuntime::instance().reset();
    ApiResponse response;

    assert(TeletextApiRuntime::instance().tryHandleGet(
        "/api/vdr/broadcast/teletext/service?backend=default&channel=C-1-1051-10301",
        response));
    assert(response.statusCode == 503);
    assert(response.body.find("teletext_runtime_unavailable") != std::string::npos);
}

}

int main()
{
    testServiceRouteSerializesNormalizedIdentity();
    testPageRouteReturnsUtf8AndProviderEvidence();
    testExplicitSubpageIsForwarded();
    testMalformedRequestsFailBeforeProviderRead();
    testUnavailableServiceIsTruthful();
    testUnrelatedRouteIsNotClaimed();
    testUnconfiguredRuntimeFailsClosed();
    return 0;
}
