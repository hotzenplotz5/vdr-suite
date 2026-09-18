#include "SuiteBridgeTeletextResolver.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace
{

constexpr std::size_t kTeletextRows = 25;
constexpr std::size_t kTeletextColumns = 40;
constexpr std::size_t kTeletextCellCount = kTeletextRows * kTeletextColumns;

bool safeIdentity(const std::string& value, std::size_t maximumLength)
{
    return !value.empty() && value.size() <= maximumLength &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isalnum(character) != 0 || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

void appendUtf8(std::string& output, std::uint32_t codepoint)
{
    if (codepoint <= 0x7fU)
    {
        output.push_back(static_cast<char>(codepoint));
    }
    else if (codepoint <= 0x7ffU)
    {
        output.push_back(static_cast<char>(0xc0U | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
    else if (codepoint <= 0xffffU)
    {
        output.push_back(static_cast<char>(0xe0U | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
    else
    {
        output.push_back(static_cast<char>(0xf0U | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 12) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | ((codepoint >> 6) & 0x3fU)));
        output.push_back(static_cast<char>(0x80U | (codepoint & 0x3fU)));
    }
}

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& input)
        : input_(input)
    {
    }

    bool consume(char expected)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != expected)
        {
            return false;
        }
        ++position_;
        return true;
    }

    bool atEnd()
    {
        skipWhitespace();
        return position_ == input_.size();
    }

    bool parseString(std::string& output)
    {
        skipWhitespace();
        if (position_ >= input_.size() || input_[position_] != '"')
        {
            return false;
        }
        ++position_;
        output.clear();

        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);
            if (character == '"')
            {
                return true;
            }
            if (character < 0x20U)
            {
                return false;
            }
            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
                continue;
            }

            if (position_ >= input_.size())
            {
                return false;
            }
            const char escape = input_[position_++];
            switch (escape)
            {
                case '"': output.push_back('"'); break;
                case '\\': output.push_back('\\'); break;
                case '/': output.push_back('/'); break;
                case 'b': output.push_back('\b'); break;
                case 'f': output.push_back('\f'); break;
                case 'n': output.push_back('\n'); break;
                case 'r': output.push_back('\r'); break;
                case 't': output.push_back('\t'); break;
                case 'u':
                {
                    std::uint32_t codepoint = 0;
                    for (int index = 0; index < 4; ++index)
                    {
                        if (position_ >= input_.size())
                        {
                            return false;
                        }
                        const char hex = input_[position_++];
                        codepoint <<= 4;
                        if (hex >= '0' && hex <= '9')
                        {
                            codepoint |= static_cast<std::uint32_t>(hex - '0');
                        }
                        else if (hex >= 'a' && hex <= 'f')
                        {
                            codepoint |= static_cast<std::uint32_t>(hex - 'a' + 10);
                        }
                        else if (hex >= 'A' && hex <= 'F')
                        {
                            codepoint |= static_cast<std::uint32_t>(hex - 'A' + 10);
                        }
                        else
                        {
                            return false;
                        }
                    }
                    if (codepoint >= 0xd800U && codepoint <= 0xdfffU)
                    {
                        return false;
                    }
                    appendUtf8(output, codepoint);
                    break;
                }
                default:
                    return false;
            }
        }
        return false;
    }

    bool parseUnsigned(std::uint64_t& output)
    {
        skipWhitespace();
        if (position_ >= input_.size() ||
            !std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            return false;
        }

        std::uint64_t value = 0;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            const unsigned int digit =
                static_cast<unsigned int>(input_[position_] - '0');
            if (value >
                (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
            {
                return false;
            }
            value = value * 10U + digit;
            ++position_;
        }
        output = value;
        return true;
    }

    bool parseBool(bool& output)
    {
        skipWhitespace();
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            output = true;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            output = false;
            return true;
        }
        return false;
    }

    bool skipValue(int depth = 0)
    {
        if (depth > 16)
        {
            return false;
        }
        skipWhitespace();
        if (position_ >= input_.size())
        {
            return false;
        }

        if (input_[position_] == '"')
        {
            std::string ignored;
            return parseString(ignored);
        }
        if (input_[position_] == '{')
        {
            ++position_;
            bool first = true;
            while (true)
            {
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == '}')
                {
                    ++position_;
                    return true;
                }
                if (!first && !consume(','))
                {
                    return false;
                }
                std::string key;
                if (!parseString(key) || !consume(':') || !skipValue(depth + 1))
                {
                    return false;
                }
                first = false;
            }
        }
        if (input_[position_] == '[')
        {
            ++position_;
            bool first = true;
            while (true)
            {
                skipWhitespace();
                if (position_ < input_.size() && input_[position_] == ']')
                {
                    ++position_;
                    return true;
                }
                if (!first && !consume(','))
                {
                    return false;
                }
                if (!skipValue(depth + 1))
                {
                    return false;
                }
                first = false;
            }
        }
        if (input_.compare(position_, 4, "true") == 0)
        {
            position_ += 4;
            return true;
        }
        if (input_.compare(position_, 5, "false") == 0)
        {
            position_ += 5;
            return true;
        }
        if (input_.compare(position_, 4, "null") == 0)
        {
            position_ += 4;
            return true;
        }

        if (input_[position_] == '-')
        {
            ++position_;
        }
        bool digitSeen = false;
        while (position_ < input_.size() &&
               std::isdigit(static_cast<unsigned char>(input_[position_])))
        {
            digitSeen = true;
            ++position_;
        }
        return digitSeen;
    }

private:
    void skipWhitespace()
    {
        while (position_ < input_.size() &&
               std::isspace(static_cast<unsigned char>(input_[position_])))
        {
            ++position_;
        }
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

struct CapabilityWire
{
    std::uint64_t schemaVersion = 0;
    std::string provider;
    std::uint64_t providerSchemaVersion = 0;
    std::string capability;
    bool available = false;
    std::uint64_t rows = 0;
    std::uint64_t columns = 0;
    bool subpages = false;
    bool level1 = false;
    bool x26Partial = false;
    bool conceal = false;
    bool blink = false;
    bool doubleSize = false;
    bool haveSchema = false;
    bool haveProvider = false;
    bool haveProviderSchema = false;
    bool haveCapability = false;
    bool haveAvailable = false;
    bool haveRows = false;
    bool haveColumns = false;
};

bool parseCapabilityPayload(const std::string& payload, CapabilityWire& wire)
{
    JsonCursor cursor(payload);
    if (!cursor.consume('{'))
    {
        return false;
    }

    bool first = true;
    while (true)
    {
        if (cursor.consume('}'))
        {
            break;
        }
        if (!first && !cursor.consume(','))
        {
            return false;
        }

        std::string key;
        if (!cursor.parseString(key) || !cursor.consume(':'))
        {
            return false;
        }

        if (key == "schemaVersion")
        {
            wire.haveSchema = cursor.parseUnsigned(wire.schemaVersion);
            if (!wire.haveSchema) return false;
        }
        else if (key == "provider")
        {
            wire.haveProvider = cursor.parseString(wire.provider);
            if (!wire.haveProvider) return false;
        }
        else if (key == "providerSchemaVersion")
        {
            wire.haveProviderSchema =
                cursor.parseUnsigned(wire.providerSchemaVersion);
            if (!wire.haveProviderSchema) return false;
        }
        else if (key == "capability")
        {
            wire.haveCapability = cursor.parseString(wire.capability);
            if (!wire.haveCapability) return false;
        }
        else if (key == "available")
        {
            wire.haveAvailable = cursor.parseBool(wire.available);
            if (!wire.haveAvailable) return false;
        }
        else if (key == "rows")
        {
            wire.haveRows = cursor.parseUnsigned(wire.rows);
            if (!wire.haveRows) return false;
        }
        else if (key == "columns")
        {
            wire.haveColumns = cursor.parseUnsigned(wire.columns);
            if (!wire.haveColumns) return false;
        }
        else if (key == "subpages")
        {
            if (!cursor.parseBool(wire.subpages)) return false;
        }
        else if (key == "level1")
        {
            if (!cursor.parseBool(wire.level1)) return false;
        }
        else if (key == "x26Partial")
        {
            if (!cursor.parseBool(wire.x26Partial)) return false;
        }
        else if (key == "conceal")
        {
            if (!cursor.parseBool(wire.conceal)) return false;
        }
        else if (key == "blink")
        {
            if (!cursor.parseBool(wire.blink)) return false;
        }
        else if (key == "doubleSize")
        {
            if (!cursor.parseBool(wire.doubleSize)) return false;
        }
        else if (!cursor.skipValue())
        {
            return false;
        }
        first = false;
    }

    return cursor.atEnd() && wire.haveSchema && wire.haveProvider &&
        wire.haveProviderSchema && wire.haveCapability && wire.haveAvailable &&
        wire.haveRows && wire.haveColumns;
}

bool parseStringArray(JsonCursor& cursor, std::vector<std::string>& output)
{
    if (!cursor.consume('['))
    {
        return false;
    }
    output.clear();
    bool first = true;
    while (true)
    {
        if (cursor.consume(']'))
        {
            return true;
        }
        if (!first && !cursor.consume(','))
        {
            return false;
        }
        std::string value;
        if (!cursor.parseString(value))
        {
            return false;
        }
        output.push_back(std::move(value));
        if (output.size() > kTeletextRows)
        {
            return false;
        }
        first = false;
    }
}

bool parseCells(JsonCursor& cursor, std::vector<TeletextCell>& output)
{
    if (!cursor.consume('['))
    {
        return false;
    }
    output.clear();
    bool firstCell = true;
    while (true)
    {
        if (cursor.consume(']'))
        {
            return true;
        }
        if (!firstCell && !cursor.consume(','))
        {
            return false;
        }
        if (!cursor.consume('['))
        {
            return false;
        }

        std::uint64_t values[7] = {};
        for (std::size_t index = 0; index < 7; ++index)
        {
            if (index != 0 && !cursor.consume(','))
            {
                return false;
            }
            if (!cursor.parseUnsigned(values[index]))
            {
                return false;
            }
        }
        if (!cursor.consume(']') || values[0] > 0x10ffffU)
        {
            return false;
        }
        for (std::size_t index = 1; index < 7; ++index)
        {
            if (values[index] > 0xffU)
            {
                return false;
            }
        }

        TeletextCell cell;
        cell.codepoint = static_cast<std::uint32_t>(values[0]);
        cell.rawChar = static_cast<std::uint8_t>(values[1]);
        cell.charset = static_cast<std::uint8_t>(values[2]);
        cell.foreground = static_cast<std::uint8_t>(values[3]);
        cell.background = static_cast<std::uint8_t>(values[4]);
        cell.kind = static_cast<std::uint8_t>(values[5]);
        cell.flags = static_cast<std::uint8_t>(values[6]);
        output.push_back(cell);
        if (output.size() > kTeletextCellCount)
        {
            return false;
        }
        firstCell = false;
    }
}

struct PageWire
{
    std::uint64_t schemaVersion = 0;
    std::string result;
    std::string channel;
    std::uint64_t requestedPage = 0;
    std::string source;
    bool receiverActive = false;
    bool teletextAvailable = false;
    std::uint64_t serviceEpoch = 0;
    std::uint64_t revision = 0;
    std::uint64_t observedAt = 0;
    std::uint64_t page = 0;
    std::uint64_t subpage = 0;
    bool complete = false;
    std::uint64_t rows = 0;
    std::uint64_t columns = 0;
    std::vector<std::string> text;
    std::vector<TeletextCell> cells;
    bool haveSchema = false;
    bool haveResult = false;
    bool haveChannel = false;
    bool haveRequestedPage = false;
    bool haveSource = false;
    bool haveReceiverActive = false;
    bool haveTeletextAvailable = false;
    bool haveServiceEpoch = false;
    bool haveRevision = false;
    bool haveObservedAt = false;
    bool havePage = false;
    bool haveSubpage = false;
    bool haveComplete = false;
    bool haveRows = false;
    bool haveColumns = false;
    bool haveText = false;
    bool haveCells = false;
};

bool parsePagePayload(const std::string& payload, PageWire& wire)
{
    JsonCursor cursor(payload);
    if (!cursor.consume('{'))
    {
        return false;
    }

    bool first = true;
    while (true)
    {
        if (cursor.consume('}'))
        {
            break;
        }
        if (!first && !cursor.consume(','))
        {
            return false;
        }

        std::string key;
        if (!cursor.parseString(key) || !cursor.consume(':'))
        {
            return false;
        }

        if (key == "schemaVersion")
        {
            wire.haveSchema = cursor.parseUnsigned(wire.schemaVersion);
            if (!wire.haveSchema) return false;
        }
        else if (key == "result")
        {
            wire.haveResult = cursor.parseString(wire.result);
            if (!wire.haveResult) return false;
        }
        else if (key == "channel")
        {
            wire.haveChannel = cursor.parseString(wire.channel);
            if (!wire.haveChannel) return false;
        }
        else if (key == "requestedPage")
        {
            wire.haveRequestedPage = cursor.parseUnsigned(wire.requestedPage);
            if (!wire.haveRequestedPage) return false;
        }
        else if (key == "source")
        {
            wire.haveSource = cursor.parseString(wire.source);
            if (!wire.haveSource) return false;
        }
        else if (key == "receiverActive")
        {
            wire.haveReceiverActive = cursor.parseBool(wire.receiverActive);
            if (!wire.haveReceiverActive) return false;
        }
        else if (key == "teletextAvailable")
        {
            wire.haveTeletextAvailable =
                cursor.parseBool(wire.teletextAvailable);
            if (!wire.haveTeletextAvailable) return false;
        }
        else if (key == "serviceEpoch")
        {
            wire.haveServiceEpoch = cursor.parseUnsigned(wire.serviceEpoch);
            if (!wire.haveServiceEpoch) return false;
        }
        else if (key == "revision")
        {
            wire.haveRevision = cursor.parseUnsigned(wire.revision);
            if (!wire.haveRevision) return false;
        }
        else if (key == "observedAt")
        {
            wire.haveObservedAt = cursor.parseUnsigned(wire.observedAt);
            if (!wire.haveObservedAt) return false;
        }
        else if (key == "page")
        {
            wire.havePage = cursor.parseUnsigned(wire.page);
            if (!wire.havePage) return false;
        }
        else if (key == "subpage")
        {
            wire.haveSubpage = cursor.parseUnsigned(wire.subpage);
            if (!wire.haveSubpage) return false;
        }
        else if (key == "complete")
        {
            wire.haveComplete = cursor.parseBool(wire.complete);
            if (!wire.haveComplete) return false;
        }
        else if (key == "rows")
        {
            wire.haveRows = cursor.parseUnsigned(wire.rows);
            if (!wire.haveRows) return false;
        }
        else if (key == "columns")
        {
            wire.haveColumns = cursor.parseUnsigned(wire.columns);
            if (!wire.haveColumns) return false;
        }
        else if (key == "text")
        {
            wire.haveText = parseStringArray(cursor, wire.text);
            if (!wire.haveText) return false;
        }
        else if (key == "cells")
        {
            wire.haveCells = parseCells(cursor, wire.cells);
            if (!wire.haveCells) return false;
        }
        else if (!cursor.skipValue())
        {
            return false;
        }
        first = false;
    }

    return cursor.atEnd() && wire.haveSchema && wire.haveResult &&
        wire.haveChannel && wire.haveRequestedPage && wire.haveSource &&
        wire.haveReceiverActive && wire.haveTeletextAvailable &&
        wire.haveServiceEpoch && wire.haveRevision && wire.haveObservedAt &&
        wire.havePage && wire.haveSubpage && wire.haveComplete &&
        wire.haveRows && wire.haveColumns;
}

TeletextSourceState sourceState(const std::string& source)
{
    if (source == "live") return TeletextSourceState::Live;
    if (source == "cached") return TeletextSourceState::Cached;
    return TeletextSourceState::Unknown;
}

}

SuiteBridgeTeletextResolver::SuiteBridgeTeletextResolver(
    ISuiteBridgeTeletextTransport& transport)
    : transport_(transport)
{
}

TeletextServiceSnapshot SuiteBridgeTeletextResolver::discoverService(
    const std::string& backendId,
    std::uint64_t backendGeneration,
    const std::string& channelId) const
{
    TeletextServiceSnapshot snapshot;
    if (!safeIdentity(backendId, 128) || backendGeneration == 0 ||
        !safeIdentity(channelId, 63))
    {
        snapshot.error = "invalid_teletext_service_context";
        return snapshot;
    }

    const SuiteBridgeTeletextCommandReply reply = transport_.discoverTeletext();
    if (!reply.transportSucceeded || reply.replyCode != 250)
    {
        snapshot.error = "teletext_provider_unreachable";
        return snapshot;
    }

    CapabilityWire wire;
    if (!parseCapabilityPayload(reply.payload, wire) || wire.schemaVersion != 1 ||
        !safeIdentity(wire.provider, 64) || wire.providerSchemaVersion == 0 ||
        wire.capability != "broadcast.teletext.page" ||
        wire.rows != kTeletextRows || wire.columns != kTeletextColumns)
    {
        snapshot.error = "teletext_capability_payload_invalid";
        return snapshot;
    }

    snapshot.payloadValid = true;
    snapshot.service.backendId = backendId;
    snapshot.service.backendGeneration = backendGeneration;
    snapshot.service.channelId = channelId;
    snapshot.service.provider.providerId = wire.provider;
    snapshot.service.provider.capabilityRevision = wire.providerSchemaVersion;
    snapshot.service.available = wire.available;
    snapshot.rows = static_cast<std::uint16_t>(wire.rows);
    snapshot.columns = static_cast<std::uint16_t>(wire.columns);
    snapshot.subpages = wire.subpages;
    snapshot.level1 = wire.level1;
    snapshot.x26Partial = wire.x26Partial;
    snapshot.conceal = wire.conceal;
    snapshot.blink = wire.blink;
    snapshot.doubleSize = wire.doubleSize;
    return snapshot;
}

TeletextPageSnapshot SuiteBridgeTeletextResolver::readPage(
    const TeletextServiceRef& service,
    std::uint16_t pageNumber,
    bool automaticSubpage,
    std::uint16_t subpageCode) const
{
    TeletextPageSnapshot snapshot;
    if (!safeIdentity(service.backendId, 128) ||
        service.backendGeneration == 0 ||
        !safeIdentity(service.channelId, 63) ||
        !safeIdentity(service.provider.providerId, 64) ||
        service.provider.capabilityRevision == 0 || !service.available ||
        pageNumber < 100 || pageNumber > 899 ||
        (!automaticSubpage && subpageCode == 0xffffU))
    {
        snapshot.error = "invalid_teletext_page_context";
        return snapshot;
    }

    SuiteBridgeTeletextPageRequest request;
    request.channelId = service.channelId;
    request.pageNumber = pageNumber;
    request.automaticSubpage = automaticSubpage;
    request.subpageCode = subpageCode;

    const SuiteBridgeTeletextCommandReply reply =
        transport_.requestTeletextPage(request);
    if (!reply.transportSucceeded || reply.replyCode != 250)
    {
        snapshot.error = "teletext_page_transport_failed";
        return snapshot;
    }

    PageWire wire;
    if (!parsePagePayload(reply.payload, wire) || wire.schemaVersion != 1 ||
        wire.channel != service.channelId || wire.requestedPage != pageNumber ||
        (wire.source != "live" && wire.source != "cached" &&
         wire.source != "unknown") ||
        wire.serviceEpoch == 0)
    {
        snapshot.error = "teletext_page_payload_invalid";
        return snapshot;
    }

    snapshot.payloadValid = true;
    snapshot.result = wire.result;
    snapshot.page.service = service;
    snapshot.page.service.available = wire.teletextAvailable;
    snapshot.page.service.receiverActive = wire.receiverActive;
    snapshot.page.service.source = sourceState(wire.source);
    snapshot.page.service.provider.providerGeneration = wire.serviceEpoch;
    snapshot.page.service.provider.observedAt = wire.observedAt;
    snapshot.page.pageNumber = pageNumber;
    snapshot.page.subpageCode = automaticSubpage ? 0 : subpageCode;
    snapshot.revision = wire.revision;
    snapshot.complete = wire.complete;
    snapshot.rows = static_cast<std::uint16_t>(wire.rows);
    snapshot.columns = static_cast<std::uint16_t>(wire.columns);

    if (wire.result != "ok")
    {
        return snapshot;
    }

    if (!wire.teletextAvailable || wire.page != pageNumber ||
        wire.page < 100 || wire.page > 899 || wire.subpage > 0xffffU ||
        wire.revision == 0 || wire.observedAt == 0 ||
        wire.rows != kTeletextRows || wire.columns != kTeletextColumns ||
        !wire.haveText || !wire.haveCells || wire.text.size() != kTeletextRows ||
        wire.cells.size() != kTeletextCellCount)
    {
        snapshot.payloadValid = false;
        snapshot.error = "teletext_page_payload_invalid";
        snapshot.result.clear();
        snapshot.textRows.clear();
        snapshot.cells.clear();
        return snapshot;
    }

    snapshot.pageAvailable = true;
    snapshot.page.pageNumber = static_cast<std::uint16_t>(wire.page);
    snapshot.page.subpageCode = static_cast<std::uint16_t>(wire.subpage);
    snapshot.textRows = std::move(wire.text);
    snapshot.cells = std::move(wire.cells);
    return snapshot;
}
