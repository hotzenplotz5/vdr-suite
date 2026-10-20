#include "AddonActivationCatalogReadService.h"

#include <utility>

namespace vdrsuite::addons {

AddonCatalogReadResult AddonActivationCatalogReadService::read(
    const RequestSecurityContext& actor,
    const std::string& backendId) const
{
    if (!canInspectInventory(actor).allowed)
        return {CatalogReadStatus::forbidden, {}};

    const IntentCatalogResult snapshot = intents_.readCatalog(actor, backendId);
    if (snapshot.status != IntentStatus::ok ||
        snapshot.intents.size() != kReviewedAddonCatalog.size())
        return {CatalogReadStatus::unavailable, {}};

    AddonCatalogReadResult result;
    for (std::size_t i = 0; i < kReviewedAddonCatalog.size(); ++i)
    {
        const auto& reviewed = kReviewedAddonCatalog[i];
        const auto& intent = snapshot.intents[i];
        if (intent.moduleId != reviewed.id || intent.backendId != backendId)
            return {CatalogReadStatus::unavailable, {}};
        AddonCatalogRecord record;
        record.moduleId = std::string(reviewed.id);
        record.packageName = std::string(reviewed.packageName);
        record.desiredEnabled = intent.desiredEnabled;
        record.revision = intent.revision;
        // No installed provider is approved or executable at this stage.
        // A persisted administrator wish does not imply active capability.
        record.active = false;
        result.modules.push_back(std::move(record));
    }
    result.status = CatalogReadStatus::ok;
    return result;
}

} // namespace vdrsuite::addons
