#include "SuiteBridgeHbbtvPresentationResolver.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace
{

std::string base64(const std::string& input)
{
    static constexpr char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string output;
    for (std::size_t offset = 0; offset < input.size(); offset += 3U)
    {
        const std::uint32_t a =
            static_cast<unsigned char>(input[offset]);
        const std::uint32_t b =
            offset + 1U < input.size()
                ? static_cast<unsigned char>(input[offset + 1U]) : 0U;
        const std::uint32_t c =
            offset + 2U < input.size()
                ? static_cast<unsigned char>(input[offset + 2U]) : 0U;
        const std::uint32_t value = (a << 16U) | (b << 8U) | c;
        output.push_back(alphabet[(value >> 18U) & 0x3fU]);
        output.push_back(alphabet[(value >> 12U) & 0x3fU]);
        output.push_back(offset + 1U < input.size()
            ? alphabet[(value >> 6U) & 0x3fU] : '=');
        output.push_back(offset + 2U < input.size()
            ? alphabet[value & 0x3fU] : '=');
    }
    return output;
}

std::string qoi1x1()
{
    std::string bytes;
    bytes.append("qoif", 4);
    bytes.append("\0\0\0\1", 4);
    bytes.append("\0\0\0\1", 4);
    bytes.push_back(4);
    bytes.push_back(0);
    bytes.push_back(static_cast<char>(0xff));
    bytes.push_back(static_cast<char>(0xff));
    bytes.push_back(0);
    bytes.push_back(0);
    bytes.push_back(static_cast<char>(0xff));
    bytes.append("\0\0\0\0\0\0\0\1", 8);
    return bytes;
}

SuiteBridgeHbbtvCommandReply reply(const std::string& payload)
{
    SuiteBridgeHbbtvCommandReply result;
    result.transportSucceeded = true;
    result.replyCode = 250;
    result.payload = payload;
    return result;
}

class FakeTransport final : public ISuiteBridgeHbbtvTransport
{
public:
    SuiteBridgeHbbtvCommandReply discoverHbbtv(
        const std::string&) override
    {
        return {};
    }

    SuiteBridgeHbbtvCommandReply readHbbtvPresentation(
        const SuiteBridgeHbbtvPresentationRequest& request) override
    {
        requests.push_back(request);
        if (request.operation == SuiteBridgeHbbtvPresentationOperation::Meta)
            return meta;
        return chunk;
    }

    std::vector<SuiteBridgeHbbtvPresentationRequest> requests;
    SuiteBridgeHbbtvCommandReply meta;
    SuiteBridgeHbbtvCommandReply chunk;
};

}

int main()
{
    const std::string image = qoi1x1();

    FakeTransport transport;
    transport.meta = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"meta\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":9,\"observedAt\":1234,"
        "\"renderWidth\":1,\"renderHeight\":1,"
        "\"encodedBytes\":" + std::to_string(image.size()) +
        ",\"returnedBytes\":0}");

    transport.chunk = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"chunk\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":9,\"observedAt\":1234,"
        "\"renderWidth\":1,\"renderHeight\":1,"
        "\"encodedBytes\":" + std::to_string(image.size()) +
        ",\"returnedBytes\":" + std::to_string(image.size()) +
        ",\"dataBase64\":\"" + base64(image) + "\"}");

    SuiteBridgeHbbtvPresentationResolver resolver(transport);
    const HbbtvPresentationFrame frame =
        resolver.readPresentation("session-a", 0);

    assert(frame.available);
    assert(!frame.unchanged);
    assert(frame.error.empty());
    assert(frame.frameRevision == 9);
    assert(frame.renderWidth == 1);
    assert(frame.renderHeight == 1);
    assert(frame.qoi == image);
    assert(transport.requests.size() == 2);
    assert(transport.requests[1].frameRevision == 9);
    assert(transport.requests[1].offset == 0);

    transport.requests.clear();
    const HbbtvPresentationFrame unchanged =
        resolver.readPresentation("session-a", 9);
    assert(unchanged.available);
    assert(unchanged.unchanged);
    assert(unchanged.qoi.empty());
    assert(transport.requests.size() == 1);

    transport.meta = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"meta\","
        "\"result\":\"no_frame\",\"resultCode\":4,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":0,\"observedAt\":0,"
        "\"renderWidth\":0,\"renderHeight\":0,"
        "\"encodedBytes\":0,\"returnedBytes\":0}");
    const HbbtvPresentationFrame noFrame =
        resolver.readPresentation("session-a", 0);
    assert(!noFrame.available);
    assert(noFrame.error.empty());

    transport.meta = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":1,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"meta\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"sessionId\":\"other\","
        "\"frameRevision\":9,\"observedAt\":1234,"
        "\"renderWidth\":1,\"renderHeight\":1,"
        "\"encodedBytes\":10,\"returnedBytes\":0}");
    const HbbtvPresentationFrame mismatch =
        resolver.readPresentation("session-a", 0);
    assert(!mismatch.available);
    assert(mismatch.error == "hbbtv_presentation_payload_invalid");

    return 0;
}
