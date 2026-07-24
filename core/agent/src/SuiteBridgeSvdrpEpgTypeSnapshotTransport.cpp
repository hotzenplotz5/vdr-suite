#include "SuiteBridgeSvdrpTransport.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
namespace
{

bool parseUnsigned(
    const std::string& value,
    unsigned long long& parsed)
{
    if (value.empty() ||
        !std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isdigit(character) != 0;
        }))
    {
        return false;
    }

    try
    {
        std::size_t consumed = 0;
        parsed = std::stoull(value, &consumed, 10);
        return consumed == value.size();
    }
    catch (...)
    {
        return false;
    }
}

std::vector<std::string> splitPreservingEmpty(
    const std::string& value,
    char delimiter)
{
    std::vector<std::string> result;
    std::size_t begin = 0;
    while (true)
    {
        const std::size_t end = value.find(delimiter, begin);
        if (end == std::string::npos)
        {
            result.push_back(value.substr(begin));
            return result;
        }
        result.push_back(value.substr(begin, end - begin));
        begin = end + 1;
    }
}

bool safeChannelId(const std::string& value)
{
    if (value.empty() || value.size() > 255)
    {
        return false;
    }
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return character > 0x20 && character != 0x7f &&
            character != ',' && character != ';' && character != '|';
    });
}

bool parsePayload(
    const std::string& payload,
    std::uint64_t requestedOffset,
    std::size_t requestedLimit,
    SuiteBridgeEpgTypeSnapshotTransportPage& page)
{
    const std::vector<std::string> sections = splitPreservingEmpty(payload, '|');
    if (sections.size() != 5 || sections[0] != "1")
    {
        return false;
    }

    unsigned long long nextOffset = 0;
    unsigned long long scanned = 0;
    if (!parseUnsigned(sections[1], nextOffset) ||
        !parseUnsigned(sections[2], scanned) ||
        (sections[3] != "0" && sections[3] != "1") ||
        scanned > requestedLimit ||
        nextOffset != requestedOffset + scanned)
    {
        return false;
    }

    page.nextOffset = static_cast<std::uint64_t>(nextOffset);
    page.scanned = static_cast<std::size_t>(scanned);
    page.done = sections[3] == "1";
    page.items.clear();

    if (sections[4].empty())
    {
        return true;
    }

    const std::vector<std::string> encodedItems =
        splitPreservingEmpty(sections[4], ';');
    if (encodedItems.size() > page.scanned)
    {
        return false;
    }

    for (const std::string& encoded : encodedItems)
    {
        const std::vector<std::string> fields =
            splitPreservingEmpty(encoded, ',');
        if (fields.size() != 5 || !safeChannelId(fields[0]))
        {
            return false;
        }

        unsigned long long eventId = 0;
        unsigned long long startTime = 0;
        unsigned long long endTime = 0;
        if (!parseUnsigned(fields[1], eventId) || eventId == 0 ||
            !parseUnsigned(fields[2], startTime) ||
            !parseUnsigned(fields[3], endTime) ||
            startTime > static_cast<unsigned long long>(
                std::numeric_limits<std::int64_t>::max()) ||
            endTime > static_cast<unsigned long long>(
                std::numeric_limits<std::int64_t>::max()) ||
            endTime <= startTime ||
            (fields[4] != "S" && fields[4] != "M"))
        {
            return false;
        }

        SuiteBridgeEpgTypeSnapshotTransportItem item;
        item.channelId = fields[0];
        item.eventId = fields[1];
        item.startTime = static_cast<std::int64_t>(startTime);
        item.endTime = static_cast<std::int64_t>(endTime);
        item.mediaType = fields[4] == "S"
            ? EpgScraperMediaType::Series
            : EpgScraperMediaType::Movie;
        page.items.push_back(std::move(item));
    }

    return true;
}

}

SuiteBridgeEpgTypeSnapshotTransportPage
SuiteBridgeSvdrpTransport::requestEpgTypeSnapshot(
    std::int64_t fromTime,
    std::int64_t untilTime,
    std::uint64_t offset,
    std::size_t limit)
{
    SuiteBridgeEpgTypeSnapshotTransportPage page;
    if (fromTime <= 0 || untilTime <= fromTime ||
        untilTime - fromTime > 72 * 60 * 60 ||
        offset > 1000000 || limit == 0 || limit > 64)
    {
        return page;
    }

    const SuiteBridgeCommandReply reply = executeRequest(
        "PLUG suitebridge ETYPES " + std::to_string(fromTime) + " " +
        std::to_string(untilTime) + " " + std::to_string(offset) + " " +
        std::to_string(limit) + "\r\n");

    page.replyCode = reply.replyCode;
    page.transportSucceeded =
        reply.transportSucceeded() && reply.replyCode == 250;
    if (!page.transportSucceeded)
    {
        return page;
    }

    page.payloadValid = parsePayload(reply.payload, offset, limit, page);
    if (!page.payloadValid)
    {
        page.items.clear();
    }
    return page;
}

}
