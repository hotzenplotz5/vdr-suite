#include "LegacyOsdDomain.h"

#include <cassert>
#include <type_traits>

int main()
{
    static_assert(std::is_copy_constructible<OsdFrame>::value);

    OsdFrame frame;
    frame.surface.backendId = "default";
    frame.surface.backendGeneration = 7;
    frame.surface.surfaceId = "primary-native-osd";
    frame.surface.osdEpoch = "0123456789abcdef0123456789abcdef";
    frame.state = OsdSurfaceState::Active;
    frame.kind = OsdFrameKind::Menu;
    frame.frameSequence = 11;
    frame.fullFrame = true;
    frame.complete = true;
    frame.items.push_back(OsdTextItem{"Recordings", true});
    frame.selectedIndex = 0;

    assert(frame.surface.backendGeneration == 7);
    assert(frame.surface.osdEpoch != std::to_string(frame.surface.backendGeneration));
    assert(frame.frameSequence == 11);
    assert(frame.fullFrame);
    assert(frame.items.size() == 1);
    assert(osdFrameWithinLimits(frame));

    frame.title.assign(OsdFrameLimits::TitleBytes + 1, 'x');
    assert(!osdFrameWithinLimits(frame));
    frame.title.clear();

    frame.items.assign(
        OsdFrameLimits::MaximumItems + 1,
        OsdTextItem{"bounded", true});
    assert(!osdFrameWithinLimits(frame));
    return 0;
}
