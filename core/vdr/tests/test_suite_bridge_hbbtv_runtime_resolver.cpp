#include "SuiteBridgeHbbtvRuntimeResolver.h"

#include <cassert>
#include <string>

namespace
{

class FakeTransport final : public ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string&) override
    {
        return {};
    }

    SuiteBridgeHbbtvCommandReply controlHbbtv(
        const SuiteBridgeHbbtvRuntimeRequest& request) override
    {
        ++calls;
        lastRequest = request;
        return reply;
    }

    int calls = 0;
    SuiteBridgeHbbtvRuntimeRequest lastRequest;
    SuiteBridgeHbbtvCommandReply reply;
};

SuiteBridgeHbbtvRuntimeRequest launchRequest()
{
    SuiteBridgeHbbtvRuntimeRequest request;
    request.operation = SuiteBridgeHbbtvRuntimeOperation::Launch;
    request.sessionId = "bas_001122";
    request.channelId = "C-1-1051-10301";
    request.applicationId = 1;
    request.descriptorRevision = 17;
    return request;
}

SuiteBridgeHbbtvCommandReply launchReply()
{
    SuiteBridgeHbbtvCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload =
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.runtime\","
        "\"operation\":\"launch\","
        "\"result\":\"accepted\",\"resultCode\":1,"
        "\"state\":\"starting\",\"stateCode\":1,"
        "\"sessionId\":\"bas_001122\","
        "\"channel\":\"C-1-1051-10301\","
        "\"applicationId\":1,"
        "\"descriptorRevision\":17}";
    return reply;
}

}

int main()
{
    FakeTransport transport;
    SuiteBridgeHbbtvRuntimeResolver resolver(transport);

    transport.reply = launchReply();
    const auto launch = resolver.control(launchRequest());

    assert(transport.calls == 1);
    assert(launch.payloadValid);
    assert(launch.error.empty());
    assert(launch.operation == "launch");
    assert(launch.result == "accepted");
    assert(launch.resultCode == 1);
    assert(launch.state == "starting");
    assert(launch.stateCode == 1);
    assert(launch.sessionId == "bas_001122");
    assert(launch.channelId == "C-1-1051-10301");
    assert(launch.applicationId == 1);
    assert(launch.descriptorRevision == 17);

    transport.reply = launchReply();
    const std::string expectedSession = "\"sessionId\":\"bas_001122\"";
    const std::size_t position = transport.reply.payload.find(expectedSession);
    assert(position != std::string::npos);
    transport.reply.payload.replace(
        position,
        expectedSession.size(),
        "\"sessionId\":\"other-session\"");

    const auto mismatched = resolver.control(launchRequest());
    assert(!mismatched.payloadValid);
    assert(mismatched.error == "hbbtv_runtime_identity_mismatch");

    SuiteBridgeHbbtvRuntimeRequest input = launchRequest();
    input.operation = SuiteBridgeHbbtvRuntimeOperation::Input;
    input.inputAction = SuiteBridgeHbbtvInputAction::Left;

    transport.reply.transportSucceeded = true;
    transport.reply.replyCode = 250;
    transport.reply.payload =
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.runtime\","
        "\"operation\":\"input\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"state\":\"active\",\"stateCode\":2,"
        "\"sessionId\":\"bas_001122\","
        "\"channel\":\"C-1-1051-10301\","
        "\"applicationId\":1,"
        "\"descriptorRevision\":17,"
        "\"action\":\"LEFT\"}";

    const auto inputResult = resolver.control(input);
    assert(inputResult.payloadValid);
    assert(inputResult.action == "LEFT");

    transport.reply.payload.replace(
        transport.reply.payload.find("\"LEFT\""),
        std::string("\"LEFT\"").size(),
        "\"VK_LEFT\"");

    const auto rawKey = resolver.control(input);
    assert(!rawKey.payloadValid);
    assert(rawKey.error == "hbbtv_runtime_action_mismatch");

    transport.reply = {};
    const auto unavailable = resolver.control(launchRequest());
    assert(!unavailable.payloadValid);
    assert(unavailable.error == "hbbtv_runtime_provider_unreachable");

    return 0;
}
