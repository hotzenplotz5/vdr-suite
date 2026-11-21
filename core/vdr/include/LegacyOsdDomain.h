#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct OsdFrameLimits
{
    static constexpr std::size_t MaximumItems = 128;
    static constexpr std::size_t TitleBytes = 256;
    static constexpr std::size_t StatusBytes = 512;
    static constexpr std::size_t ButtonBytes = 128;
    static constexpr std::size_t ItemTextBytes = 512;
    static constexpr std::size_t DetailTextBytes = 4096;
    static constexpr std::size_t ChannelBytes = 512;
    static constexpr std::size_t ProgrammeTextBytes = 512;
};

enum class OsdSurfaceState
{
    Inactive,
    Active,
    Degraded,
    Suppressed
};

enum class OsdFrameKind
{
    None,
    Menu,
    ChannelInfo
};

struct OsdSurfaceRef
{
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::string surfaceId;
    std::string osdEpoch;
};

struct OsdTextItem
{
    std::string text;
    bool selectable = true;
};

struct OsdProgrammeInfo
{
    std::int64_t presentTime = 0;
    std::string presentTitle;
    std::string presentSubtitle;
    std::int64_t followingTime = 0;
    std::string followingTitle;
    std::string followingSubtitle;
};

struct OsdFrame
{
    OsdSurfaceRef surface;
    OsdSurfaceState state = OsdSurfaceState::Inactive;
    OsdFrameKind kind = OsdFrameKind::None;
    std::uint64_t frameSequence = 0;
    std::uint64_t observedAt = 0;
    bool fullFrame = true;
    bool complete = true;

    std::string title;
    std::string statusMessage;
    std::string red;
    std::string green;
    std::string yellow;
    std::string blue;
    std::vector<OsdTextItem> items;
    int selectedIndex = -1;
    std::string text;
    std::string channel;
    OsdProgrammeInfo programme;
};

inline bool osdFrameWithinLimits(const OsdFrame& frame) noexcept
{
    if (frame.title.size() > OsdFrameLimits::TitleBytes ||
        frame.statusMessage.size() > OsdFrameLimits::StatusBytes ||
        frame.red.size() > OsdFrameLimits::ButtonBytes ||
        frame.green.size() > OsdFrameLimits::ButtonBytes ||
        frame.yellow.size() > OsdFrameLimits::ButtonBytes ||
        frame.blue.size() > OsdFrameLimits::ButtonBytes ||
        frame.text.size() > OsdFrameLimits::DetailTextBytes ||
        frame.channel.size() > OsdFrameLimits::ChannelBytes ||
        frame.items.size() > OsdFrameLimits::MaximumItems ||
        frame.programme.presentTitle.size() > OsdFrameLimits::ProgrammeTextBytes ||
        frame.programme.presentSubtitle.size() > OsdFrameLimits::ProgrammeTextBytes ||
        frame.programme.followingTitle.size() > OsdFrameLimits::ProgrammeTextBytes ||
        frame.programme.followingSubtitle.size() > OsdFrameLimits::ProgrammeTextBytes)
    {
        return false;
    }

    for (const OsdTextItem& item : frame.items)
    {
        if (item.text.size() > OsdFrameLimits::ItemTextBytes)
        {
            return false;
        }
    }

    return true;
}
