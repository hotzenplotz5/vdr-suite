#pragma once

#include "AddonCatalog.h"
#include <array>
#include <string_view>
#include <vector>

namespace vdrsuite::addons {

// Core-owned permission vocabulary: never parsed from addon.json.
struct AddonPermissionDefinition {
    std::string_view moduleId;
    std::string_view permission;
    std::string_view presentationKey;
    bool grantable;
};

inline constexpr std::array<AddonPermissionDefinition, 3> kAddonPermissions{{
    {"rectools", "addons.media.import", "addons.media.import", true},
    {"rectools", "addons.media.check", "addons.media.check", false},
    {"rectools", "addons.media.shrink", "addons.media.shrink", true},
}};

struct AddonPermissionView {
    std::string_view moduleId;
    std::string_view packageName;
    std::string_view permission;
    std::string_view presentationKey;
    bool selectable = false;
};

// An installed manifest cannot introduce grants. Only reviewed, actually
// enabled runtime capabilities may be selectable. Caller-provided status
// is PRESENTATION ONLY; the authorization service always rechecks grants.
inline std::vector<AddonPermissionView> permissionSettingsFor(
    std::string_view moduleId,
    bool installedAndApproved,
    bool runtimeActive)
{
    std::vector<AddonPermissionView> result;
    const auto* module = reviewedAddon(moduleId);
    if (!module) return result;
    for (const auto& definition : kAddonPermissions) {
        if (definition.moduleId != moduleId) continue;
        result.push_back({module->id, module->packageName,
            definition.permission, definition.presentationKey,
            installedAndApproved && runtimeActive && definition.grantable});
    }
    return result;
}

inline bool reviewedAddonPermission(std::string_view permission)
{
    for (const auto& entry : kAddonPermissions)
        if (entry.permission == permission && entry.grantable)
            return true;
    return false;
}
} // namespace vdrsuite::addons
