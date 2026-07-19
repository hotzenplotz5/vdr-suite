#include "SuiteBridgeEpgArtworkResolver.h"

#include <cassert>

class FakeTransport final : public ISuiteBridgeArtworkTransport
{
public:
    SuiteBridgeArtworkCommandReply reply;
    std::string channel;
    std::string event;

    SuiteBridgeArtworkCommandReply requestArtwork(
        const std::string& channelId,
        const std::string& eventId) override
    {
        channel = channelId;
        event = eventId;
        return reply;
    }
};

static VdrEvent makeEvent()
{
    VdrEvent event;
    event.channelId = "S19.2E-1-1011-11100";
    event.id = "12345";
    event.title = "Example";
    return event;
}

int main()
{
    FakeTransport transport;
    SuiteBridgeEpgArtworkResolver resolver(transport);

    transport.reply.transportSucceeded = true;
    transport.reply.replyCode = 250;
    transport.reply.payload =
        "{\"schema\":1,\"found\":true,\"provider\":\"tvscraper\","
        "\"path\":\"/var/cache/tvscraper/example.jpg\","
        "\"width\":1280,\"height\":720}";

    EpgArtworkResolution found = resolver.resolve("home", makeEvent());
    assert(found.attempted);
    assert(found.found);
    assert(found.artwork.valid());
    assert(found.artwork.backendId == "home");
    assert(found.artwork.path == "/var/cache/tvscraper/example.jpg");
    assert(transport.channel == "S19.2E-1-1011-11100");
    assert(transport.event == "12345");

    transport.reply.payload =
        "{\"schema\":1,\"found\":false,\"provider\":\"none\","
        "\"path\":\"\",\"width\":0,\"height\":0}";
    EpgArtworkResolution missing = resolver.resolve("home", makeEvent());
    assert(missing.attempted);
    assert(!missing.found);

    transport.reply.transportSucceeded = false;
    EpgArtworkResolution unavailable = resolver.resolve("home", makeEvent());
    assert(!unavailable.attempted);

    transport.reply.transportSucceeded = true;
    transport.reply.payload = "not-json";
    EpgArtworkResolution malformed = resolver.resolve("home", makeEvent());
    assert(!malformed.attempted);

    return 0;
}
