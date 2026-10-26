#include "AddonPermissionCatalog.h"
#include <cassert>

int main()
{
    using namespace vdrsuite::addons;
    auto absent = permissionSettingsFor("rectools", false, false);
    assert(absent.size() == 3);
    for (const auto& item : absent) assert(!item.selectable);
    auto inactive = permissionSettingsFor("rectools", true, false);
    for (const auto& item : inactive) assert(!item.selectable);
    auto active = permissionSettingsFor("rectools", true, true);
    assert(active.size() == 3);
    assert(active[0].permission == "addons.media.import");
    assert(active[0].selectable);
    assert(!active[1].selectable && active[2].selectable);
    assert(reviewedAddonPermission("addons.media.import"));
    assert(!reviewedAddonPermission("addons.media.check"));
    assert(reviewedAddonPermission("addons.media.shrink"));
    assert(!reviewedAddonPermission("addons.unknown.action"));
    assert(permissionSettingsFor("image", true, true).empty());
    assert(permissionSettingsFor("music", true, true).empty());
    assert(permissionSettingsFor("unknown", true, true).empty());
}
