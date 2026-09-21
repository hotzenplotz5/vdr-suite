#include "SuiteBridgeOsdFrameParser.h"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

class Cursor
{
public:
    explicit Cursor(const std::string& text)
        : text_(text)
    {
    }

    bool consume(char expected)
    {
        skipWhitespace();
        if (position_ >= text_.size() || text_[position_] != expected)
        {
            return fail(std::string("expected '") + expected + "'");
        }
        ++position_;
        return true;
    }

    bool consumeIf(char expected)
    {
        skipWhitespace();
        if (position_ < text_.size() && text_[position_] == expected)
        {
            ++position_;
            return true;
        }
        return false;
    }

    bool key(const char* expected)
    {
        std::string actual;
        return string(actual, 64) &&
            actual == expected &&
            consume(':');
    }

    bool string(std::string& output, std::size_t maximumBytes)
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
                return output.size() <= maximumBytes ||
                    fail("JSON string exceeds bounded size");
            }

            if (character < 0x20)
            {
                return fail("control character in JSON string");
            }

            if (character != '\\')
            {
                output.push_back(static_cast<char>(character));
            }
            else
            {
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
                        std::uint32_t codePoint = 0;
                        for (int index = 0; index < 4; ++index)
                        {
                            if (position_ >= text_.size())
                            {
                                return fail("incomplete Unicode escape");
                            }
                            const int value = hexValue(text_[position_++]);
                            if (value < 0)
                            {
                                return fail("invalid Unicode escape");
                            }
                            codePoint =
                                (codePoint << 4) |
                                static_cast<std::uint32_t>(value);
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

            if (output.size() > maximumBytes)
            {
                return fail("JSON string exceeds bounded size");
            }
        }

        return fail("unterminated JSON string");
    }

    bool boolean(bool& output)
    {
        skipWhitespace();
        if (literal("true"))
        {
            output = true;
            return true;
        }
        if (literal("false"))
        {
            output = false;
            return true;
        }
        return fail("expected boolean");
    }

    bool unsignedInteger(std::uint64_t& output)
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
            return fail("leading zero in integer");
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

    bool signedInteger(std::int64_t& output)
    {
        skipWhitespace();
        bool negative = false;
        if (position_ < text_.size() && text_[position_] == '-')
        {
            negative = true;
            ++position_;
        }

        std::uint64_t magnitude = 0;
        if (!unsignedInteger(magnitude))
        {
            return false;
        }

        const std::uint64_t positiveMaximum =
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max());
        const std::uint64_t negativeMaximum = positiveMaximum + 1U;

        if ((!negative && magnitude > positiveMaximum) ||
            (negative && magnitude > negativeMaximum))
        {
            return fail("signed integer overflow");
        }

        if (!negative)
        {
            output = static_cast<std::int64_t>(magnitude);
        }
        else if (magnitude == negativeMaximum)
        {
            output = std::numeric_limits<std::int64_t>::min();
        }
        else
        {
            output = -static_cast<std::int64_t>(magnitude);
        }

        return true;
    }

    bool finished()
    {
        skipWhitespace();
        return position_ == text_.size();
    }

    const std::string& error() const
    {
        return error_;
    }

private:
    void skipWhitespace()
    {
        while (position_ < text_.size())
        {
            const char character = text_[position_];
            if (character != ' ' &&
                character != '\t' &&
                character != '\r' &&
                character != '\n')
            {
                break;
            }
            ++position_;
        }
    }

    bool literal(const char* value)
    {
        const std::size_t start = position_;
        for (; *value != '\0'; ++value, ++position_)
        {
            if (position_ >= text_.size() ||
                text_[position_] != *value)
            {
                position_ = start;
                return false;
            }
        }
        return true;
    }

    bool fail(const std::string& message)
    {
        if (error_.empty())
        {
            error_ = message;
        }
        return false;
    }

    static int hexValue(char character)
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

    static void appendUtf8(
        std::string& output,
        std::uint32_t codePoint)
    {
        if (codePoint <= 0x7F)
        {
            output.push_back(static_cast<char>(codePoint));
        }
        else if (codePoint <= 0x7FF)
        {
            output.push_back(
                static_cast<char>(0xC0 | (codePoint >> 6)));
            output.push_back(
                static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
        else
        {
            output.push_back(
                static_cast<char>(0xE0 | (codePoint >> 12)));
            output.push_back(
                static_cast<char>(
                    0x80 | ((codePoint >> 6) & 0x3F)));
            output.push_back(
                static_cast<char>(0x80 | (codePoint & 0x3F)));
        }
    }

    const std::string& text_;
    std::size_t position_ = 0;
    std::string error_;
};

SuiteBridgeOsdFrameParseResult failure(
    SuiteBridgeOsdFrameParseStatus status,
    std::string diagnostic)
{
    SuiteBridgeOsdFrameParseResult result;
    result.status = status;
    result.diagnostic = std::move(diagnostic);
    return result;
}

bool validEpoch(const std::string& value)
{
    if (value.size() != 32)
    {
        return false;
    }

    for (const char character : value)
    {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f')))
        {
            return false;
        }
    }

    return true;
}

bool parseKind(
    const std::string& value,
    OsdFrameKind& kind)
{
    if (value == "inactive")
    {
        kind = OsdFrameKind::None;
        return true;
    }
    if (value == "menu")
    {
        kind = OsdFrameKind::Menu;
        return true;
    }
    if (value == "channel_info")
    {
        kind = OsdFrameKind::ChannelInfo;
        return true;
    }
    return false;
}

bool commaAndKey(Cursor& cursor, const char* key)
{
    return cursor.consume(',') && cursor.key(key);
}

}

SuiteBridgeOsdFrameParseResult SuiteBridgeOsdFrameParser::parse(
    const std::string& payload,
    const std::string& backendId,
    std::uint64_t backendGeneration) const
{
    if (payload.size() > MaximumPayloadBytes)
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::PayloadTooLarge,
            "OSD snapshot payload exceeds bounded size");
    }

    if (backendId.empty() || backendId.size() > 128)
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "invalid backend identity for OSD frame");
    }

    Cursor cursor(payload);
    std::uint64_t schema = 0;
    bool active = false;
    bool complete = false;
    bool consistent = false;
    std::string kindText;
    std::uint64_t frameSequence = 0;
    std::uint64_t observedAt = 0;
    std::uint64_t droppedUpdates = 0;
    std::string osdEpoch;

    OsdFrame frame;
    frame.surface.backendId = backendId;
    frame.surface.backendGeneration = backendGeneration;
    frame.surface.surfaceId = "primary-native-osd";
    frame.fullFrame = true;

    if (!cursor.consume('{') ||
        !cursor.key("osd_schema") ||
        !cursor.unsignedInteger(schema) ||
        !commaAndKey(cursor, "active") ||
        !cursor.boolean(active) ||
        !commaAndKey(cursor, "complete") ||
        !cursor.boolean(complete) ||
        !commaAndKey(cursor, "consistent") ||
        !cursor.boolean(consistent) ||
        !commaAndKey(cursor, "kind") ||
        !cursor.string(kindText, 32) ||
        !commaAndKey(cursor, "frame_sequence") ||
        !cursor.unsignedInteger(frameSequence) ||
        !commaAndKey(cursor, "observed_at_ms") ||
        !cursor.unsignedInteger(observedAt) ||
        !commaAndKey(cursor, "dropped_updates") ||
        !cursor.unsignedInteger(droppedUpdates) ||
        !commaAndKey(cursor, "osd_epoch") ||
        !cursor.string(osdEpoch, 32) ||
        !commaAndKey(cursor, "title") ||
        !cursor.string(frame.title, OsdFrameLimits::TitleBytes) ||
        !commaAndKey(cursor, "status") ||
        !cursor.string(
            frame.statusMessage,
            OsdFrameLimits::StatusBytes) ||
        !commaAndKey(cursor, "red") ||
        !cursor.string(frame.red, OsdFrameLimits::ButtonBytes) ||
        !commaAndKey(cursor, "green") ||
        !cursor.string(frame.green, OsdFrameLimits::ButtonBytes) ||
        !commaAndKey(cursor, "yellow") ||
        !cursor.string(frame.yellow, OsdFrameLimits::ButtonBytes) ||
        !commaAndKey(cursor, "blue") ||
        !cursor.string(frame.blue, OsdFrameLimits::ButtonBytes) ||
        !commaAndKey(cursor, "items") ||
        !cursor.consume('['))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidJson,
            cursor.error());
    }

    if (!cursor.consumeIf(']'))
    {
        while (true)
        {
            if (frame.items.size() >= OsdFrameLimits::MaximumItems)
            {
                return failure(
                    SuiteBridgeOsdFrameParseStatus::InvalidFrame,
                    "too many OSD items");
            }

            OsdTextItem item;
            bool selectable = false;
            if (!cursor.consume('{') ||
                !cursor.key("text") ||
                !cursor.string(
                    item.text,
                    OsdFrameLimits::ItemTextBytes) ||
                !commaAndKey(cursor, "selectable") ||
                !cursor.boolean(selectable) ||
                !cursor.consume('}'))
            {
                return failure(
                    SuiteBridgeOsdFrameParseStatus::InvalidJson,
                    cursor.error());
            }
            item.selectable = selectable;
            frame.items.push_back(std::move(item));

            if (cursor.consumeIf(']'))
            {
                break;
            }
            if (!cursor.consume(','))
            {
                return failure(
                    SuiteBridgeOsdFrameParseStatus::InvalidJson,
                    cursor.error());
            }
        }
    }

    std::int64_t selectedIndex = -1;
    std::int64_t presentTime = 0;
    std::int64_t followingTime = 0;

    if (!commaAndKey(cursor, "selected_index") ||
        !cursor.signedInteger(selectedIndex) ||
        !commaAndKey(cursor, "text") ||
        !cursor.string(frame.text, OsdFrameLimits::DetailTextBytes) ||
        !commaAndKey(cursor, "channel") ||
        !cursor.string(frame.channel, OsdFrameLimits::ChannelBytes) ||
        !commaAndKey(cursor, "programme") ||
        !cursor.consume('{') ||
        !cursor.key("present_time") ||
        !cursor.signedInteger(presentTime) ||
        !commaAndKey(cursor, "present_title") ||
        !cursor.string(
            frame.programme.presentTitle,
            OsdFrameLimits::ProgrammeTextBytes) ||
        !commaAndKey(cursor, "present_subtitle") ||
        !cursor.string(
            frame.programme.presentSubtitle,
            OsdFrameLimits::ProgrammeTextBytes) ||
        !commaAndKey(cursor, "following_time") ||
        !cursor.signedInteger(followingTime) ||
        !commaAndKey(cursor, "following_title") ||
        !cursor.string(
            frame.programme.followingTitle,
            OsdFrameLimits::ProgrammeTextBytes) ||
        !commaAndKey(cursor, "following_subtitle") ||
        !cursor.string(
            frame.programme.followingSubtitle,
            OsdFrameLimits::ProgrammeTextBytes) ||
        !cursor.consume('}') ||
        !cursor.consume('}') ||
        !cursor.finished())
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidJson,
            cursor.error().empty()
                ? "trailing OSD snapshot payload data"
                : cursor.error());
    }

    if (schema != 1)
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::UnsupportedSchema,
            "unsupported OSD snapshot schema");
    }

    if (!parseKind(kindText, frame.kind))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "invalid OSD frame kind");
    }

    if ((active && frame.kind == OsdFrameKind::None) ||
        (!active && frame.kind != OsdFrameKind::None))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "OSD active state and frame kind disagree");
    }

    if (active && !validEpoch(osdEpoch))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "active OSD frame has invalid epoch");
    }

    if (!active && !osdEpoch.empty() && !validEpoch(osdEpoch))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "inactive OSD frame has invalid epoch");
    }

    if (selectedIndex < -1 ||
        selectedIndex >=
            static_cast<std::int64_t>(OsdFrameLimits::MaximumItems) ||
        (selectedIndex >= 0 &&
         static_cast<std::size_t>(selectedIndex) >= frame.items.size()))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "invalid OSD selected item index");
    }

    frame.surface.osdEpoch = std::move(osdEpoch);
    frame.frameSequence = frameSequence;
    frame.observedAt = observedAt;
    frame.complete = complete && consistent;
    frame.selectedIndex = static_cast<int>(selectedIndex);
    frame.programme.presentTime = presentTime;
    frame.programme.followingTime = followingTime;

    if (!active)
    {
        frame.state = OsdSurfaceState::Inactive;
    }
    else if (!complete || !consistent)
    {
        frame.state = OsdSurfaceState::Degraded;
    }
    else
    {
        frame.state = OsdSurfaceState::Active;
    }

    if (!osdFrameWithinLimits(frame))
    {
        return failure(
            SuiteBridgeOsdFrameParseStatus::InvalidFrame,
            "OSD frame exceeds domain limits");
    }

    SuiteBridgeOsdFrameParseResult result;
    result.status = SuiteBridgeOsdFrameParseStatus::Ok;
    result.value.frame = std::move(frame);
    result.value.sourceConsistent = consistent;
    result.value.droppedUpdates = droppedUpdates;
    return result;
}

}
