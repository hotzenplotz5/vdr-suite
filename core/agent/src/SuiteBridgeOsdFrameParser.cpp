#include "SuiteBridgeOsdFrameParser.h"
#include "OsdJsonCursor.h"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace vdrsuite::agent
{
namespace
{

using osd_json::Cursor;

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
