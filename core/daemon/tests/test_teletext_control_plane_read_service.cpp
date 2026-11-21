#include "BackendRegistry.h"
#include "SnapshotAccessService.h"
#include "SnapshotCache.h"
#include "SnapshotCacheService.h"
#include "SuiteBridgeTeletextResolver.h"
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
        const std::string& backendId,
        std::int64_t now) const override
    {
        lastBackendId = backendId;
        lastNow = now;
        if (states.empty())
        {
            return {};
        }
        const std::size_t index =
            calls < states.size() ? calls : states.size() - 1;
        ++calls;
        return states[index];
    }

    void reset(std::vector<TeletextBackendAuthorityState> values)
    {
        states = std::move(values);
        calls = 0;
        lastBackendId.clear();
        lastNow = 0;
    }

    mutable std::size_t calls = 0;
    mutable std::string lastBackendId;
    mutable std::int64_t lastNow = 0;
    std::vector<TeletextBackendAuthorityState> states;
};

TeletextBackendAuthorityState onlineGeneration(std::uint64_t generation)
{
    TeletextBackendAuthorityState state;
    state.present = true;
    state.online = true;
    state.backendGeneration = generation;
    return state;
}

SuiteBridgeTeletextCommandReply capabilityReply()
{
    SuiteBridgeTeletextCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload =
        "{\"schemaVersion\":1,\"provider\":\"osdteletext\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.teletext.page\","
        "\"available\":true,\"rows\":25,\"columns\":40,"
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
        if (row == 0)
        {
            json << "\"Börse und Nachrichten\"";
        }
        else
        {
            json << "\"Teletext row\"";
        }
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

VdrSnapshot makeSnapshot(const std::string& channelId)
{
    VdrSnapshot snapshot;
    snapshot.backendId = "default";
    snapshot.status.enabled = true;
    snapshot.status.state = "connected";

    VdrChannel channel;
    channel.id = channelId;
    channel.name = "Das Erste HD";
    snapshot.channels.push_back(channel);
    return snapshot;
}

TeletextServiceRef serviceRef(std::uint64_t generation)
{
    TeletextServiceRef service;
    service.backendId = "default";
    service.backendGeneration = generation;
    service.channelId = "C-1-1051-10301";
    service.provider.providerId = "osdteletext";
    service.provider.capabilityRevision = 1;
    service.available = true;
    return service;
}

struct Fixture
{
    Fixture()
        : registryService(registry),
          cacheService(cache),
          accessService(cacheService),
          snapshotReadService(accessService),
          resolver(transport)
    {
        BackendNode backend;
        backend.backendId = "default";
        backend.enabled = true;
        registry.addBackend(backend);

        cache.updateForBackend(
            "default",
            makeSnapshot("C-1-1051-10301"));

        transport.capabilityReply = capabilityReply();
        transport.pageReply = pageReply();
        authority.reset({onlineGeneration(7)});

        service = std::make_unique<TeletextControlPlaneReadService>(
            registryService,
            snapshotReadService,
            authority,
            [this](const std::string& backendId) -> SuiteBridgeTeletextResolver* {
                return backendId == "default" ? &resolver : nullptr;
            },
            []() -> std::int64_t { return 4242; });
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
    std::unique_ptr<TeletextControlPlaneReadService> service;
};

void testDiscoveryUsesAuthoritativeGenerationAndChannelFence()
{
    Fixture fixture;

    const TeletextServiceSnapshot snapshot = fixture.service->discoverService(
        "default",
        "C-1-1051-10301");

    assert(snapshot.payloadValid);
    assert(snapshot.service.available);
    assert(snapshot.service.backendId == "default");
    assert(snapshot.service.backendGeneration == 7);
    assert(snapshot.service.channelId == "C-1-1051-10301");
    assert(snapshot.service.provider.providerId == "osdteletext");
    assert(fixture.transport.discoverCalls == 1);
    assert(fixture.authority.calls == 2);
    assert(fixture.authority.lastBackendId == "default");
    assert(fixture.authority.lastNow == 4242);
}

void testPageReadPreservesNormalizedProviderEvidence()
{
    Fixture fixture;
    fixture.authority.reset({onlineGeneration(7)});

    const TeletextPageSnapshot page = fixture.service->readPage(
        serviceRef(7),
        100,
        true,
        0);

    assert(page.payloadValid);
    assert(page.pageAvailable);
    assert(page.result == "ok");
    assert(page.page.service.backendGeneration == 7);
    assert(page.page.service.provider.providerGeneration == 9);
    assert(page.page.service.provider.observedAt == 12345);
    assert(page.revision == 17);
    assert(page.rows == 25);
    assert(page.columns == 40);
    assert(page.textRows.size() == 25);
    assert(page.textRows.front() == "Börse und Nachrichten");
    assert(page.cells.size() == 1000);
    assert(fixture.transport.pageCalls == 1);
    assert(fixture.transport.lastRequest.channelId == "C-1-1051-10301");
    assert(fixture.transport.lastRequest.pageNumber == 100);
    assert(fixture.transport.lastRequest.automaticSubpage);
}

void testGenerationMismatchFailsBeforeProviderRead()
{
    Fixture fixture;
    fixture.authority.reset({onlineGeneration(8)});

    const TeletextPageSnapshot page = fixture.service->readPage(
        serviceRef(7),
        100);

    assert(!page.payloadValid);
    assert(page.error == "teletext_backend_generation_mismatch");
    assert(fixture.transport.pageCalls == 0);
}

void testGenerationChangeDuringReadFailsClosed()
{
    Fixture fixture;
    fixture.authority.reset({onlineGeneration(7), onlineGeneration(8)});

    const TeletextPageSnapshot page = fixture.service->readPage(
        serviceRef(7),
        100);

    assert(!page.payloadValid);
    assert(!page.pageAvailable);
    assert(page.error == "teletext_backend_generation_mismatch");
    assert(fixture.transport.pageCalls == 1);
}

void testUnknownChannelFailsBeforeProviderDiscovery()
{
    Fixture fixture;

    const TeletextServiceSnapshot snapshot = fixture.service->discoverService(
        "default",
        "C-1-1051-99999");

    assert(!snapshot.payloadValid);
    assert(snapshot.error == "teletext_channel_not_in_backend_snapshot");
    assert(fixture.transport.discoverCalls == 0);
    assert(fixture.authority.calls == 0);
}

void testOfflineBackendFailsBeforeProviderDiscovery()
{
    Fixture fixture;
    TeletextBackendAuthorityState offline = onlineGeneration(7);
    offline.online = false;
    fixture.authority.reset({offline});

    const TeletextServiceSnapshot snapshot = fixture.service->discoverService(
        "default",
        "C-1-1051-10301");

    assert(!snapshot.payloadValid);
    assert(snapshot.error == "teletext_backend_not_online");
    assert(fixture.transport.discoverCalls == 0);
}

}

int main()
{
    testDiscoveryUsesAuthoritativeGenerationAndChannelFence();
    testPageReadPreservesNormalizedProviderEvidence();
    testGenerationMismatchFailsBeforeProviderRead();
    testGenerationChangeDuringReadFailsClosed();
    testUnknownChannelFailsBeforeProviderDiscovery();
    testOfflineBackendFailsBeforeProviderDiscovery();
    return 0;
}
