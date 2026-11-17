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

std::string wire(
    const std::string& operation,
    const std::string& kind,
    std::uint64_t kindCode,
    std::uint64_t revision,
    std::uint64_t baseRevision,
    std::uint32_t renderWidth,
    std::uint32_t renderHeight,
    std::uint32_t patchX,
    std::uint32_t patchY,
    std::uint32_t patchWidth,
    std::uint32_t patchHeight,
    const std::string& image,
    bool chunk)
{
    return
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":2,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"" + operation + "\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":" + std::to_string(revision) + ","
        "\"observedAt\":1234,"
        "\"frameKind\":\"" + kind + "\","
        "\"frameKindCode\":" + std::to_string(kindCode) + ","
        "\"baseRevision\":" + std::to_string(baseRevision) + ","
        "\"renderWidth\":" + std::to_string(renderWidth) + ","
        "\"renderHeight\":" + std::to_string(renderHeight) + ","
        "\"patchX\":" + std::to_string(patchX) + ","
        "\"patchY\":" + std::to_string(patchY) + ","
        "\"patchWidth\":" + std::to_string(patchWidth) + ","
        "\"patchHeight\":" + std::to_string(patchHeight) + ","
        "\"encodedBytes\":" + std::to_string(image.size()) + ","
        "\"returnedBytes\":" +
            std::to_string(chunk ? image.size() : 0U) +
        (chunk
            ? ",\"dataBase64\":\"" + base64(image) + "\"}"
            : "}");
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

} // namespace

int main()
{
    const std::string image = qoi1x1();

    FakeTransport transport;
    transport.meta = reply(wire(
        "meta", "full", 1, 9, 0, 1, 1, 0, 0, 1, 1, image, false));
    transport.chunk = reply(wire(
        "chunk", "full", 1, 9, 0, 1, 1, 0, 0, 1, 1, image, true));

    SuiteBridgeHbbtvPresentationResolver resolver(transport);
    const HbbtvPresentationFrame full =
        resolver.readPresentation("session-a", 0);

    assert(full.available);
    assert(!full.unchanged);
    assert(!full.patch);
    assert(full.error.empty());
    assert(full.frameRevision == 9);
    assert(full.renderWidth == 1);
    assert(full.renderHeight == 1);
    assert(full.patchWidth == 1);
    assert(full.patchHeight == 1);
    assert(full.qoi == image);
    assert(transport.requests.size() == 2);
    assert(transport.requests[0].frameRevision == 0);
    assert(transport.requests[1].frameRevision == 9);
    assert(transport.requests[1].offset == 0);

    transport.requests.clear();
    transport.meta = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":2,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"meta\","
        "\"result\":\"ok\",\"resultCode\":0,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":9,\"observedAt\":1234,"
        "\"frameKind\":\"none\",\"frameKindCode\":0,"
        "\"baseRevision\":9,"
        "\"renderWidth\":1,\"renderHeight\":1,"
        "\"patchX\":0,\"patchY\":0,"
        "\"patchWidth\":0,\"patchHeight\":0,"
        "\"encodedBytes\":0,\"returnedBytes\":0}");
    const HbbtvPresentationFrame unchanged =
        resolver.readPresentation("session-a", 9);
    assert(unchanged.available);
    assert(unchanged.unchanged);
    assert(unchanged.qoi.empty());
    assert(transport.requests.size() == 1);
    assert(transport.requests[0].frameRevision == 9);

    transport.requests.clear();
    transport.meta = reply(wire(
        "meta", "patch", 2, 10, 9, 2, 1, 1, 0, 1, 1, image, false));
    transport.chunk = reply(wire(
        "chunk", "patch", 2, 10, 9, 2, 1, 1, 0, 1, 1, image, true));
    const HbbtvPresentationFrame patch =
        resolver.readPresentation("session-a", 9);
    assert(patch.available);
    assert(!patch.unchanged);
    assert(patch.patch);
    assert(patch.baseRevision == 9);
    assert(patch.frameRevision == 10);
    assert(patch.renderWidth == 2);
    assert(patch.renderHeight == 1);
    assert(patch.patchX == 1);
    assert(patch.patchY == 0);
    assert(patch.patchWidth == 1);
    assert(patch.patchHeight == 1);
    assert(patch.qoi == image);
    assert(transport.requests.size() == 2);
    assert(transport.requests[0].frameRevision == 9);

    transport.meta = reply(
        "{\"schemaVersion\":1,"
        "\"provider\":\"vdr-plugin-web\","
        "\"providerSchemaVersion\":2,"
        "\"capability\":\"broadcast.hbbtv.presentation\","
        "\"operation\":\"meta\","
        "\"result\":\"no_frame\",\"resultCode\":4,"
        "\"sessionId\":\"session-a\","
        "\"frameRevision\":0,\"observedAt\":0,"
        "\"frameKind\":\"none\",\"frameKindCode\":0,"
        "\"baseRevision\":0,"
        "\"renderWidth\":0,\"renderHeight\":0,"
        "\"patchX\":0,\"patchY\":0,"
        "\"patchWidth\":0,\"patchHeight\":0,"
        "\"encodedBytes\":0,\"returnedBytes\":0}");
    const HbbtvPresentationFrame noFrame =
        resolver.readPresentation("session-a", 0);
    assert(!noFrame.available);
    assert(noFrame.error.empty());

    return 0;
}
