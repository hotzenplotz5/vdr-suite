#include "SuiteBridgeHbbtvMediaResolver.h"

#include <cassert>
#include <string>

namespace
{
SuiteBridgeHbbtvCommandReply reply(const std::string& payload)
{
    SuiteBridgeHbbtvCommandReply r;
    r.transportSucceeded = true;
    r.replyCode = 250;
    r.payload = payload;
    return r;
}
class FakeTransport final : public ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgeHbbtvCommandReply discoverHbbtv(const std::string&) override { return {}; }
    SuiteBridgeHbbtvCommandReply readHbbtvMedia(const SuiteBridgeHbbtvMediaRequest& request) override
    {
        lastSession = request.sessionId;
        return response;
    }
    std::string lastSession;
    SuiteBridgeHbbtvCommandReply response;
};
}

int main()
{
    FakeTransport transport;
    transport.response = reply(
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,\"capability\":\"broadcast.hbbtv.media\","
        "\"result\":\"ok\",\"resultCode\":0,\"sessionId\":\"session-a\","
        "\"state\":\"streaming\",\"stateCode\":1,\"mediaRevision\":3,"
        "\"fullscreen\":false,\"consumerConnected\":false,"
        "\"x\":100,\"y\":50,\"width\":640,\"height\":360,"
        "\"socketPath\":\"/run/vdr/vdr-suite-hbbtv-media/m-123-3.sock\"}");
    SuiteBridgeHbbtvMediaResolver resolver(transport);
    auto media = resolver.resolveMedia("session-a");
    assert(media.available && media.error.empty());
    assert(media.state == HbbtvMediaSourceState::Streaming);
    assert(media.mediaRevision == 3 && !media.fullscreen && !media.consumerConnected);
    assert(media.x == 100 && media.y == 50 && media.width == 640 && media.height == 360);
    assert(media.unixSocketPath == "/run/vdr/vdr-suite-hbbtv-media/m-123-3.sock");
    assert(transport.lastSession == "session-a");

    transport.response = reply(
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,\"capability\":\"broadcast.hbbtv.media\","
        "\"result\":\"ok\",\"resultCode\":0,\"sessionId\":\"session-a\","
        "\"state\":\"stopped\",\"stateCode\":3,\"mediaRevision\":3,"
        "\"fullscreen\":true,\"consumerConnected\":false,"
        "\"x\":0,\"y\":0,\"width\":0,\"height\":0,\"socketPath\":\"\"}");
    media = resolver.resolveMedia("session-a");
    assert(!media.available && media.error.empty());
    assert(media.state == HbbtvMediaSourceState::Stopped);

    transport.response = reply(
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,\"capability\":\"broadcast.hbbtv.media\","
        "\"result\":\"no_session\",\"resultCode\":2,\"sessionId\":\"session-a\","
        "\"state\":\"none\",\"stateCode\":0,\"mediaRevision\":0,"
        "\"fullscreen\":true,\"consumerConnected\":false,"
        "\"x\":0,\"y\":0,\"width\":0,\"height\":0,\"socketPath\":\"\"}");
    media = resolver.resolveMedia("session-a");
    assert(!media.available && media.error.empty());

    transport.response = reply(
        "{\"schemaVersion\":1,\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,\"capability\":\"broadcast.hbbtv.media\","
        "\"result\":\"ok\",\"resultCode\":0,\"sessionId\":\"session-a\","
        "\"state\":\"streaming\",\"stateCode\":1,\"mediaRevision\":3,"
        "\"fullscreen\":false,\"consumerConnected\":false,"
        "\"x\":0,\"y\":0,\"width\":640,\"height\":360,"
        "\"socketPath\":\"/tmp/../escape.sock\"}");
    media = resolver.resolveMedia("session-a");
    assert(!media.available && media.error == "hbbtv_media_payload_invalid");

    media = resolver.resolveMedia("bad/session");
    assert(!media.available && media.error == "hbbtv_media_request_invalid");
    return 0;
}
