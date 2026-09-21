#pragma once

#include <cstdint>
#include <string>
#include <vector>

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
