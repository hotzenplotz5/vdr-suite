#pragma once

#include <array>
#include <string_view>

namespace vdrsuite::addons {

struct AddonCatalogIdentity {
    std::string_view id;
    std::string_view packageName;
};

inline constexpr std::array<AddonCatalogIdentity, 3> kReviewedAddonCatalog{{
    {"rectools", "vdr-suite-addon-media-tools"},
    {"image", "vdr-suite-addon-image"},
    {"music", "vdr-suite-addon-music"},
}};

inline constexpr const AddonCatalogIdentity* reviewedAddon(std::string_view id)
{
    for (const auto& item : kReviewedAddonCatalog)
        if (item.id == id) return &item;
    return nullptr;
}

} // namespace vdrsuite::addons
