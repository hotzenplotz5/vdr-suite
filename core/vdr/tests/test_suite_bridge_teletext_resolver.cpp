#include "SuiteBridgeTeletextResolver.h"

#include <cassert>
#include <cstdint>
#include <sstream>
#include <string>

namespace
{

class MockTeletextTransport final : public ISuiteBridgeTeletextTransport
{
public:
    SuiteBridgeTeletextCommandReply capabilityReply;
    SuiteBridgeTeletextCommandReply pageReply;
    int discoverCalls = 0;
    int pageCalls = 0;
    SuiteBridgeTeletextPageRequest lastRequest;

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
};

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
        "\"maxSnapshots\":2048,\"subpages\":true,"
        "\"level1\":true,\"x26Partial\":true,"
        "\"conceal\":true,\"blink\":true,\"doubleSize\":true,"
        "\"flof\":false,\"topNavigation\":false}";
    return reply;
}

std::string pagePayload(
    const std::string& channel,
    const std::string& result,
    const std::string& source,
    bool receiverActive,
    bool teletextAvailable,
    std::uint64_t serviceEpoch,
    std::uint64_t revision,
    std::uint64_t observedAt,
    std::uint16_t resolvedPage,
    std::uint16_t subpage,
    bool includePageData = true,
    std::size_t cellCount = 1000)
{
    std::ostringstream out;
    out << "{\"schemaVersion\":1,\"result\":\"" << result << "\""
        << ",\"resultCode\":0"
        << ",\"channel\":\"" << channel << "\""
        << ",\"requestedPage\":100"
        << ",\"requestedSubpage\":\"auto\""
        << ",\"source\":\"" << source << "\""
        << ",\"receiverActive\":" << (receiverActive ? "true" : "false")
        << ",\"teletextAvailable\":" << (teletextAvailable ? "true" : "false")
        << ",\"serviceEpoch\":" << serviceEpoch
        << ",\"revision\":" << revision
        << ",\"observedAt\":" << observedAt
        << ",\"page\":" << resolvedPage
        << ",\"subpage\":" << subpage
        << ",\"complete\":false"
        << ",\"rows\":" << (includePageData ? 25 : 0)
        << ",\"columns\":" << (includePageData ? 40 : 0);

    if (includePageData)
    {
        out << ",\"text\":[";
        for (int row = 0; row < 25; ++row)
        {
            if (row != 0) out << ',';
            if (row == 0)
            {
                out << "\"Börse fällt und lügt nicht\"";
            }
            else
            {
                out << "\"\"";
            }
        }
        out << "],\"cells\":[";
        for (std::size_t index = 0; index < cellCount; ++index)
        {
            if (index != 0) out << ',';
            out << "[32,32,0,7,0,0,0]";
        }
        out << ']';
    }
    out << '}';
    return out.str();
}

SuiteBridgeTeletextCommandReply pageReply(const std::string& payload)
{
    SuiteBridgeTeletextCommandReply reply;
    reply.transportSucceeded = true;
    reply.replyCode = 250;
    reply.payload = payload;
    return reply;
}

TeletextServiceRef discoverService(
    MockTeletextTransport& transport,
    SuiteBridgeTeletextResolver& resolver)
{
    transport.capabilityReply = capabilityReply();
    const TeletextServiceSnapshot service = resolver.discoverService(
        "backend-main",
        7,
        "C-1-1051-10301");

    assert(service.payloadValid);
    assert(service.error.empty());
    assert(service.service.backendId == "backend-main");
    assert(service.service.backendGeneration == 7);
    assert(service.service.channelId == "C-1-1051-10301");
    assert(service.service.provider.providerId == "osdteletext");
    assert(service.service.provider.capabilityRevision == 1);
    assert(service.service.available);
    assert(service.rows == 25);
    assert(service.columns == 40);
    assert(service.subpages);
    assert(service.level1);
    assert(service.x26Partial);
    assert(service.conceal);
    assert(service.blink);
    assert(service.doubleSize);
    return service.service;
}

}

int main()
{
    MockTeletextTransport transport;
    SuiteBridgeTeletextResolver resolver(transport);

    const TeletextServiceRef service = discoverService(transport, resolver);
    assert(transport.discoverCalls == 1);

    transport.pageReply = pageReply(pagePayload(
        service.channelId,
        "ok",
        "live",
        true,
        true,
        9,
        55,
        1789670000,
        100,
        0));

    const TeletextPageSnapshot live = resolver.readPage(service, 100);
    assert(transport.pageCalls == 1);
    assert(transport.lastRequest.channelId == service.channelId);
    assert(transport.lastRequest.pageNumber == 100);
    assert(transport.lastRequest.automaticSubpage);
    assert(live.payloadValid);
    assert(live.pageAvailable);
    assert(live.result == "ok");
    assert(live.page.service.backendId == "backend-main");
    assert(live.page.service.backendGeneration == 7);
    assert(live.page.service.provider.capabilityRevision == 1);
    assert(live.page.service.provider.providerGeneration == 9);
    assert(live.page.service.provider.observedAt == 1789670000);
    assert(live.page.service.receiverActive);
    assert(live.page.service.source == TeletextSourceState::Live);
    assert(live.page.pageNumber == 100);
    assert(live.page.subpageCode == 0);
    assert(live.revision == 55);
    assert(live.rows == 25);
    assert(live.columns == 40);
    assert(live.textRows.size() == 25);
    assert(live.textRows[0] == "Börse fällt und lügt nicht");
    assert(live.cells.size() == 1000);
    assert(live.cells[0].codepoint == 32);
    assert(live.cells[0].foreground == 7);

    transport.pageReply = pageReply(pagePayload(
        service.channelId,
        "ok",
        "cached",
        false,
        true,
        9,
        56,
        1789670001,
        100,
        3));
    const TeletextPageSnapshot cached = resolver.readPage(service, 100);
    assert(cached.payloadValid);
    assert(cached.pageAvailable);
    assert(!cached.page.service.receiverActive);
    assert(cached.page.service.source == TeletextSourceState::Cached);
    assert(cached.page.subpageCode == 3);

    transport.pageReply = pageReply(pagePayload(
        service.channelId,
        "page_not_found",
        "live",
        true,
        true,
        9,
        0,
        0,
        0,
        0,
        false));
    const TeletextPageSnapshot missing = resolver.readPage(service, 100);
    assert(missing.payloadValid);
    assert(!missing.pageAvailable);
    assert(missing.result == "page_not_found");

    transport.pageReply = pageReply(pagePayload(
        service.channelId,
        "ok",
        "live",
        true,
        true,
        10,
        57,
        1789670002,
        100,
        0,
        true,
        999));
    const TeletextPageSnapshot malformed = resolver.readPage(service, 100);
    assert(!malformed.payloadValid);
    assert(!malformed.pageAvailable);
    assert(malformed.error == "teletext_page_payload_invalid");

    const int callsBeforeInvalid = transport.pageCalls;
    TeletextServiceRef invalid = service;
    invalid.backendGeneration = 0;
    const TeletextPageSnapshot rejected = resolver.readPage(invalid, 100);
    assert(!rejected.payloadValid);
    assert(rejected.error == "invalid_teletext_page_context");
    assert(transport.pageCalls == callsBeforeInvalid);

    const TeletextServiceSnapshot invalidDiscovery = resolver.discoverService(
        "backend-main",
        0,
        service.channelId);
    assert(!invalidDiscovery.payloadValid);
    assert(invalidDiscovery.error == "invalid_teletext_service_context");

    return 0;
}
