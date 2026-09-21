#include "SuiteBridgeHbbtvResolver.h"

#include <cassert>
#include <string>

namespace
{

class MockTransport final : public ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgeHbbtvCommandReply reply;
    int calls = 0;
    std::string lastChannel;

    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string& channelId) override
    {
        ++calls;
        lastChannel = channelId;
        return reply;
    }
};

SuiteBridgeHbbtvCommandReply successReply()
{
    SuiteBridgeHbbtvCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload =
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.discovery\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"channel\":\"C-1-1051-10301\","
        "\"receiverActive\":true,\"revision\":42,"
        "\"observedAt\":1789670000,"
        "\"applications\":["
        "{\"applicationId\":7,\"controlCode\":2,\"priority\":5,"
        "\"name\":\"ARD HbbTV\","
        "\"urlBase\":\"https://example.invalid/\","
        "\"urlLocation\":\"index.html\","
        "\"urlExtension\":\"\"}]}";
    return reply;
}

}

int main()
{
    MockTransport transport;
    SuiteBridgeHbbtvResolver resolver(transport);

    transport.reply = successReply();
    const BroadcastApplicationDiscoverySnapshot snapshot =
        resolver.discoverApplications(
            "backend-main",
            7,
            "C-1-1051-10301");

    assert(transport.calls == 1);
    assert(transport.lastChannel == "C-1-1051-10301");
    assert(snapshot.payloadValid);
    assert(snapshot.error.empty());
    assert(snapshot.receiverActive);
    assert(snapshot.result == "ok");
    assert(snapshot.revision == 42);
    assert(snapshot.observedAt == 1789670000);
    assert(snapshot.applications.size() == 1);

    const BroadcastApplicationDescriptor& application =
        snapshot.applications.front();
    assert(application.ref.backendId == "backend-main");
    assert(application.ref.backendGeneration == 7);
    assert(application.ref.channelId == "C-1-1051-10301");
    assert(application.ref.provider.providerId == "vdr-plugin-web");
    assert(application.ref.provider.providerSchemaVersion == 1);
    assert(application.ref.provider.capabilityRevision == 42);
    assert(application.ref.applicationId == 7);
    assert(application.ref.descriptorRevision == 42);
    assert(application.controlCode == 2);
    assert(application.priority == 5);
    assert(application.name == "ARD HbbTV");

    transport.reply.payload =
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.discovery\","
        "\"result\":\"no_applications\",\"resultCode\":4,"
        "\"channel\":\"C-1-1051-10301\","
        "\"receiverActive\":true,\"revision\":43,"
        "\"observedAt\":1789670001,\"applications\":[]}";
    const auto empty = resolver.discoverApplications(
        "backend-main", 7, "C-1-1051-10301");
    assert(empty.payloadValid);
    assert(empty.result == "no_applications");
    assert(empty.applications.empty());

    transport.reply = successReply();
    transport.reply.payload.replace(
        transport.reply.payload.find("\"revision\":42"),
        std::string("\"revision\":42").size(),
        "\"revision\":0");
    const auto stale = resolver.discoverApplications(
        "backend-main", 7, "C-1-1051-10301");
    assert(!stale.payloadValid);
    assert(stale.error == "hbbtv_discovery_payload_invalid");

    const int beforeInvalid = transport.calls;
    const auto invalid = resolver.discoverApplications(
        "backend-main", 0, "C-1-1051-10301");
    assert(!invalid.payloadValid);
    assert(invalid.error == "hbbtv_discovery_request_invalid");
    assert(transport.calls == beforeInvalid);

    return 0;
}
