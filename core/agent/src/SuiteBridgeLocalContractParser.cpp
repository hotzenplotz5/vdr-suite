#include "SuiteBridgeLocalContractParser.h"

#include <cstdint>
#include <limits>
#include <set>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

bool isJsonWhitespace(const char character)
{
    return character == ' ' ||
           character == '\t' ||
           character == '\r' ||
           character == '\n';
}

int hexValue(const char character)
{
    if (character >= '0' && character <= '9')
    {
        return character - '0';
    }

    if (character >= 'a' && character <= 'f')
    {
        return 10 + character - 'a';
    }

    if (character >= 'A' && character <= 'F')
    {
        return 10 + character - 'A';
    }

    return -1;
}

void appendUtf8(
    std::string& output,
    const std::uint32_t codePoint)
{
    if (codePoint <= 0x7F)
    {
        output.push_back(static_cast<char>(codePoint));
    }
    else if (codePoint <= 0x7FF)
    {
        output.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
    else
    {
        output.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        output.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

class JsonCursor
{
public:
    explicit JsonCursor(const std::string& text)
        : text_(text)
    {
    }

    void skipWhitespace()
    {
        while (position_ < text_.size() &&
               isJsonWhitespace(text_[position_]))
        {
            ++position_;
        }
    }

    bool consume(const char expected)
    {
        skipWhitespace();

        if (position_ >= text_.size() ||
            text_[position_] != expected)
        {
            return fail(std::string("expected '") + expected + "'");
        }

        ++position_;
        return true;
    }

    bool consumeIf(const char expected)
    {
        skipWhitespace();

        if (position_ < text_.size() &&
            text_[position_] == expected)
        {
            ++position_;
            return true;
        }

        return false;
    }

    bool finished()
    {
        skipWhitespace();
        return position_ == text_.size();
    }

    bool parseString(std::string& output)
    {
        skipWhitespace();

        if (position_ >= text_.size() || text_[position_] != '"')
        {
            return fail("expected JSON string");
        }

        ++position_;
        output.clear();

        while (position_ < text_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(text_[position_++]);

            if (character == '"')
            {
                return true;
            }

            if (character < 0x20)
            {
                return fail("control character in JSON string");
            }

            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
                continue;
            }

            if (position_ >= text_.size())
            {
                return fail("unterminated JSON escape");
            }

            const char escaped = text_[position_++];

            switch (escaped)
            {
                case '"':
                case '\\':
                case '/':
                    output.push_back(escaped);
                    break;
                case 'b':
                    output.push_back('\b');
                    break;
                case 'f':
                    output.push_back('\f');
                    break;
                case 'n':
                    output.push_back('\n');
                    break;
                case 'r':
                    output.push_back('\r');
                    break;
                case 't':
                    output.push_back('\t');
                    break;
                case 'u':
                {
                    if (position_ + 4 > text_.size())
                    {
                        return fail("incomplete Unicode escape");
                    }

                    std::uint32_t codePoint = 0;

                    for (int index = 0; index < 4; ++index)
                    {
                        const int digit = hexValue(text_[position_++]);

                        if (digit < 0)
                        {
                            return fail("invalid Unicode escape");
                        }

                        codePoint =
                            (codePoint << 4) |
                            static_cast<std::uint32_t>(digit);
                    }

                    if (codePoint >= 0xD800 && codePoint <= 0xDFFF)
                    {
                        return fail("surrogate Unicode escape unsupported");
                    }

                    appendUtf8(output, codePoint);
                    break;
                }
                default:
                    return fail("invalid JSON escape");
            }
        }

        return fail("unterminated JSON string");
    }

    bool parseUnsigned(std::uint64_t& output)
    {
        skipWhitespace();

        if (position_ >= text_.size() ||
            text_[position_] < '0' ||
            text_[position_] > '9')
        {
            return fail("expected unsigned integer");
        }

        if (text_[position_] == '0' &&
            position_ + 1 < text_.size() &&
            text_[position_ + 1] >= '0' &&
            text_[position_ + 1] <= '9')
        {
            return fail("leading zero in JSON integer");
        }

        std::uint64_t value = 0;

        while (position_ < text_.size() &&
               text_[position_] >= '0' &&
               text_[position_] <= '9')
        {
            const std::uint64_t digit =
                static_cast<std::uint64_t>(text_[position_] - '0');

            if (value >
                (std::numeric_limits<std::uint64_t>::max() - digit) / 10)
            {
                return fail("unsigned integer overflow");
            }

            value = value * 10 + digit;
            ++position_;
        }

        output = value;
        return true;
    }

    bool parseBoolean(bool& output)
    {
        skipWhitespace();

        if (matchLiteral("true"))
        {
            output = true;
            return true;
        }

        if (matchLiteral("false"))
        {
            output = false;
            return true;
        }

        return fail("expected boolean");
    }

    bool skipValue(const int depth = 0)
    {
        if (depth > 12)
        {
            return fail("JSON nesting limit exceeded");
        }

        skipWhitespace();

        if (position_ >= text_.size())
        {
            return fail("missing JSON value");
        }

        const char character = text_[position_];

        if (character == '"')
        {
            std::string ignored;
            return parseString(ignored);
        }

        if (character == '{')
        {
            ++position_;
            skipWhitespace();

            if (consumeIf('}'))
            {
                return true;
            }

            while (true)
            {
                std::string ignoredKey;

                if (!parseString(ignoredKey) ||
                    !consume(':') ||
                    !skipValue(depth + 1))
                {
                    return false;
                }

                if (consumeIf('}'))
                {
                    return true;
                }

                if (!consume(','))
                {
                    return false;
                }
            }
        }

        if (character == '[')
        {
            ++position_;
            skipWhitespace();

            if (consumeIf(']'))
            {
                return true;
            }

            while (true)
            {
                if (!skipValue(depth + 1))
                {
                    return false;
                }

                if (consumeIf(']'))
                {
                    return true;
                }

                if (!consume(','))
                {
                    return false;
                }
            }
        }

        if (character == 't')
        {
            return matchLiteral("true") || fail("invalid literal");
        }

        if (character == 'f')
        {
            return matchLiteral("false") || fail("invalid literal");
        }

        if (character == 'n')
        {
            return matchLiteral("null") || fail("invalid literal");
        }

        return skipNumber();
    }

    const std::string& error() const
    {
        return error_;
    }

    bool fail(const std::string& message)
    {
        if (error_.empty())
        {
            error_ = message;
        }

        return false;
    }

private:
    bool matchLiteral(const char* literal)
    {
        const std::size_t start = position_;

        while (*literal != '\0')
        {
            if (position_ >= text_.size() ||
                text_[position_] != *literal)
            {
                position_ = start;
                return false;
            }

            ++position_;
            ++literal;
        }

        return true;
    }

    bool skipNumber()
    {
        skipWhitespace();
        const std::size_t start = position_;

        if (position_ < text_.size() && text_[position_] == '-')
        {
            ++position_;
        }

        if (position_ >= text_.size())
        {
            position_ = start;
            return fail("invalid JSON number");
        }

        if (text_[position_] == '0')
        {
            ++position_;
        }
        else if (text_[position_] >= '1' && text_[position_] <= '9')
        {
            while (position_ < text_.size() &&
                   text_[position_] >= '0' &&
                   text_[position_] <= '9')
            {
                ++position_;
            }
        }
        else
        {
            position_ = start;
            return fail("invalid JSON number");
        }

        if (position_ < text_.size() && text_[position_] == '.')
        {
            ++position_;
            const std::size_t fractionStart = position_;

            while (position_ < text_.size() &&
                   text_[position_] >= '0' &&
                   text_[position_] <= '9')
            {
                ++position_;
            }

            if (position_ == fractionStart)
            {
                return fail("invalid JSON fraction");
            }
        }

        if (position_ < text_.size() &&
            (text_[position_] == 'e' || text_[position_] == 'E'))
        {
            ++position_;

            if (position_ < text_.size() &&
                (text_[position_] == '+' || text_[position_] == '-'))
            {
                ++position_;
            }

            const std::size_t exponentStart = position_;

            while (position_ < text_.size() &&
                   text_[position_] >= '0' &&
                   text_[position_] <= '9')
            {
                ++position_;
            }

            if (position_ == exponentStart)
            {
                return fail("invalid JSON exponent");
            }
        }

        return true;
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;
};

SuiteBridgeCapabilityState capabilityStateFromText(
    const std::string& state)
{
    if (state == "available")
    {
        return SuiteBridgeCapabilityState::Available;
    }

    if (state == "disabled")
    {
        return SuiteBridgeCapabilityState::Disabled;
    }

    if (state == "unavailable")
    {
        return SuiteBridgeCapabilityState::Unavailable;
    }

    return SuiteBridgeCapabilityState::Unknown;
}

bool parseCapabilityObject(
    JsonCursor& cursor,
    SuiteBridgeCapabilityObservation& capability)
{
    if (!cursor.consume('{'))
    {
        return false;
    }

    bool idSeen = false;
    bool stateSeen = false;

    if (cursor.consumeIf('}'))
    {
        return cursor.fail("empty capability object");
    }

    while (true)
    {
        std::string key;

        if (!cursor.parseString(key) || !cursor.consume(':'))
        {
            return false;
        }

        if (key == "id")
        {
            if (idSeen || !cursor.parseString(capability.id))
            {
                return cursor.fail("invalid or duplicate capability id");
            }

            idSeen = true;
        }
        else if (key == "state")
        {
            if (stateSeen || !cursor.parseString(capability.rawState))
            {
                return cursor.fail("invalid or duplicate capability state");
            }

            stateSeen = true;
        }
        else if (!cursor.skipValue())
        {
            return false;
        }

        if (cursor.consumeIf('}'))
        {
            break;
        }

        if (!cursor.consume(','))
        {
            return false;
        }
    }

    if (!idSeen || !stateSeen || capability.id.empty())
    {
        return cursor.fail("capability id and state are required");
    }

    if (capability.id.size() > 128 || capability.rawState.size() > 64)
    {
        return cursor.fail("capability field too large");
    }

    capability.state = capabilityStateFromText(capability.rawState);
    return true;
}

bool parseCapabilities(
    JsonCursor& cursor,
    std::vector<SuiteBridgeCapabilityObservation>& capabilities)
{
    if (!cursor.consume('['))
    {
        return false;
    }

    if (cursor.consumeIf(']'))
    {
        return true;
    }

    std::set<std::string> observedIds;

    while (true)
    {
        if (capabilities.size() >=
            SuiteBridgeLocalContractParser::MaximumCapabilities)
        {
            return cursor.fail("too many capabilities");
        }

        SuiteBridgeCapabilityObservation capability;

        if (!parseCapabilityObject(cursor, capability))
        {
            return false;
        }

        if (!observedIds.insert(capability.id).second)
        {
            return cursor.fail("duplicate capability id");
        }

        capabilities.push_back(std::move(capability));

        if (cursor.consumeIf(']'))
        {
            return true;
        }

        if (!cursor.consume(','))
        {
            return false;
        }
    }
}

bool validEpoch(const std::string& epoch)
{
    if (epoch.size() != 32)
    {
        return false;
    }

    for (const char character : epoch)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }

    return true;
}

std::uint64_t saturatingAdd(
    const std::uint64_t left,
    const std::uint64_t right)
{
    const std::uint64_t maximum =
        std::numeric_limits<std::uint64_t>::max();

    if (left > maximum - right)
    {
        return maximum;
    }

    return left + right;
}

SuiteBridgeDiscoveryParseResult discoveryFailure(
    const SuiteBridgeParseStatus status,
    const std::string& diagnostic)
{
    SuiteBridgeDiscoveryParseResult result;
    result.status = status;
    result.diagnostic = diagnostic;
    return result;
}

SuiteBridgeSnapshotParseResult snapshotFailure(
    const SuiteBridgeParseStatus status,
    const std::string& diagnostic)
{
    SuiteBridgeSnapshotParseResult result;
    result.status = status;
    result.diagnostic = diagnostic;
    return result;
}

}

SuiteBridgeDiscoveryParseResult SuiteBridgeLocalContractParser::parseDiscovery(
    const std::string& payload) const
{
    if (payload.size() > MaximumPayloadBytes)
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::PayloadTooLarge,
            "discovery payload exceeds bounded size");
    }

    JsonCursor cursor(payload);
    SuiteBridgeDiscovery value;

    bool discoverySchemaSeen = false;
    bool pluginNameSeen = false;
    bool pluginVersionSeen = false;
    bool capabilitySchemaSeen = false;
    bool snapshotSchemaSeen = false;
    bool localContractSchemaSeen = false;
    bool capabilitiesSeen = false;

    if (!cursor.consume('{'))
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::InvalidJson,
            cursor.error());
    }

    if (cursor.consumeIf('}'))
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::MissingField,
            "discovery object is empty");
    }

    while (true)
    {
        std::string key;

        if (!cursor.parseString(key) || !cursor.consume(':'))
        {
            return discoveryFailure(
                SuiteBridgeParseStatus::InvalidJson,
                cursor.error());
        }

        bool parsed = true;

        if (key == "discovery_schema")
        {
            parsed = !discoverySchemaSeen &&
                     cursor.parseUnsigned(value.discoverySchema);
            discoverySchemaSeen = true;
        }
        else if (key == "plugin_name")
        {
            parsed = !pluginNameSeen &&
                     cursor.parseString(value.pluginName);
            pluginNameSeen = true;
        }
        else if (key == "plugin_version")
        {
            parsed = !pluginVersionSeen &&
                     cursor.parseString(value.pluginVersion);
            pluginVersionSeen = true;
        }
        else if (key == "capability_schema")
        {
            parsed = !capabilitySchemaSeen &&
                     cursor.parseUnsigned(value.capabilitySchema);
            capabilitySchemaSeen = true;
        }
        else if (key == "snapshot_schema")
        {
            parsed = !snapshotSchemaSeen &&
                     cursor.parseUnsigned(value.snapshotSchema);
            snapshotSchemaSeen = true;
        }
        else if (key == "local_contract_schema")
        {
            parsed = !localContractSchemaSeen &&
                     cursor.parseUnsigned(value.localContractSchema);
            localContractSchemaSeen = true;
        }
        else if (key == "capabilities")
        {
            parsed = !capabilitiesSeen &&
                     parseCapabilities(cursor, value.capabilities);
            capabilitiesSeen = true;
        }
        else
        {
            parsed = cursor.skipValue();
        }

        if (!parsed)
        {
            return discoveryFailure(
                SuiteBridgeParseStatus::InvalidField,
                cursor.error().empty()
                    ? "duplicate or invalid discovery field"
                    : cursor.error());
        }

        if (cursor.consumeIf('}'))
        {
            break;
        }

        if (!cursor.consume(','))
        {
            return discoveryFailure(
                SuiteBridgeParseStatus::InvalidJson,
                cursor.error());
        }
    }

    if (!cursor.finished())
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::InvalidJson,
            "trailing discovery payload data");
    }

    if (!discoverySchemaSeen ||
        !pluginNameSeen ||
        !pluginVersionSeen ||
        !capabilitySchemaSeen ||
        !snapshotSchemaSeen ||
        !localContractSchemaSeen ||
        !capabilitiesSeen)
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::MissingField,
            "required discovery field missing");
    }

    if (value.pluginName.empty() ||
        value.pluginName.size() > 64 ||
        value.pluginVersion.empty() ||
        value.pluginVersion.size() > 64)
    {
        return discoveryFailure(
            SuiteBridgeParseStatus::InvalidField,
            "invalid plugin identity field");
    }

    SuiteBridgeDiscoveryParseResult result;
    result.status = SuiteBridgeParseStatus::Ok;
    result.value = std::move(value);
    return result;
}

SuiteBridgeSnapshotParseResult SuiteBridgeLocalContractParser::parseSnapshot(
    const std::string& payload) const
{
    if (payload.size() > MaximumPayloadBytes)
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::PayloadTooLarge,
            "snapshot payload exceeds bounded size");
    }

    JsonCursor cursor(payload);
    SuiteBridgeSnapshotBaseline value;

    bool contractSchemaSeen = false;
    bool capabilitySchemaSeen = false;
    bool snapshotSchemaSeen = false;
    bool activeSeen = false;
    bool totalSeen = false;
    bool channelSwitchSeen = false;
    bool recordingSeen = false;
    bool replayingSeen = false;
    bool timerChangeSeen = false;
    bool counterEpochSeen = false;
    bool counterOverflowSeen = false;

    if (!cursor.consume('{'))
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::InvalidJson,
            cursor.error());
    }

    if (cursor.consumeIf('}'))
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::MissingField,
            "snapshot object is empty");
    }

    while (true)
    {
        std::string key;

        if (!cursor.parseString(key) || !cursor.consume(':'))
        {
            return snapshotFailure(
                SuiteBridgeParseStatus::InvalidJson,
                cursor.error());
        }

        bool parsed = true;

        if (key == "contract_schema")
        {
            parsed = !contractSchemaSeen &&
                     cursor.parseUnsigned(value.contractSchema);
            contractSchemaSeen = true;
        }
        else if (key == "capability_schema")
        {
            parsed = !capabilitySchemaSeen &&
                     cursor.parseUnsigned(value.capabilitySchema);
            capabilitySchemaSeen = true;
        }
        else if (key == "snapshot_schema")
        {
            parsed = !snapshotSchemaSeen &&
                     cursor.parseUnsigned(value.snapshotSchema);
            snapshotSchemaSeen = true;
        }
        else if (key == "active")
        {
            parsed = !activeSeen && cursor.parseBoolean(value.active);
            activeSeen = true;
        }
        else if (key == "total")
        {
            parsed = !totalSeen && cursor.parseUnsigned(value.total);
            totalSeen = true;
        }
        else if (key == "channel_switch")
        {
            parsed = !channelSwitchSeen &&
                     cursor.parseUnsigned(value.channelSwitch);
            channelSwitchSeen = true;
        }
        else if (key == "recording")
        {
            parsed = !recordingSeen &&
                     cursor.parseUnsigned(value.recording);
            recordingSeen = true;
        }
        else if (key == "replaying")
        {
            parsed = !replayingSeen &&
                     cursor.parseUnsigned(value.replaying);
            replayingSeen = true;
        }
        else if (key == "timer_change")
        {
            parsed = !timerChangeSeen &&
                     cursor.parseUnsigned(value.timerChange);
            timerChangeSeen = true;
        }
        else if (key == "counter_epoch")
        {
            parsed = !counterEpochSeen &&
                     cursor.parseString(value.counterEpoch);
            counterEpochSeen = true;
        }
        else if (key == "counter_overflow")
        {
            parsed = !counterOverflowSeen &&
                     cursor.parseBoolean(value.counterOverflow);
            counterOverflowSeen = true;
        }
        else
        {
            parsed = cursor.skipValue();
        }

        if (!parsed)
        {
            return snapshotFailure(
                SuiteBridgeParseStatus::InvalidField,
                cursor.error().empty()
                    ? "duplicate or invalid snapshot field"
                    : cursor.error());
        }

        if (cursor.consumeIf('}'))
        {
            break;
        }

        if (!cursor.consume(','))
        {
            return snapshotFailure(
                SuiteBridgeParseStatus::InvalidJson,
                cursor.error());
        }
    }

    if (!cursor.finished())
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::InvalidJson,
            "trailing snapshot payload data");
    }

    if (!contractSchemaSeen ||
        !capabilitySchemaSeen ||
        !snapshotSchemaSeen ||
        !activeSeen ||
        !totalSeen ||
        !channelSwitchSeen ||
        !recordingSeen ||
        !replayingSeen ||
        !timerChangeSeen ||
        !counterEpochSeen ||
        !counterOverflowSeen)
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::MissingField,
            "required snapshot field missing");
    }

    if (!validEpoch(value.counterEpoch))
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::InvalidField,
            "invalid counter epoch");
    }

    std::uint64_t calculatedTotal = 0;
    calculatedTotal = saturatingAdd(calculatedTotal, value.channelSwitch);
    calculatedTotal = saturatingAdd(calculatedTotal, value.recording);
    calculatedTotal = saturatingAdd(calculatedTotal, value.replaying);
    calculatedTotal = saturatingAdd(calculatedTotal, value.timerChange);

    if (value.total != calculatedTotal)
    {
        return snapshotFailure(
            SuiteBridgeParseStatus::InvalidField,
            "snapshot total does not match counters");
    }

    SuiteBridgeSnapshotParseResult result;
    result.status = SuiteBridgeParseStatus::Ok;
    result.value = std::move(value);
    return result;
}

}
