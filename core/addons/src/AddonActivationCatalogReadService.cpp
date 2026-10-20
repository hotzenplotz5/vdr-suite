#include "AddonActivationCatalogReadService.h"

namespace vdrsuite::addons {

AddonCatalogReadResult AddonActivationCatalogReadService::read(
    const RequestSecurityContext& actor,
    const std::string& backendId) const
{
    if (!canInspectInventory(actor).allowed)
        return {CatalogReadStatus::forbidden, {}};

    AddonCatalogReadResult result;
    for (const auto& reviewed : kReviewedAddonCatalog)
    {
        const IntentResult intent = intents_.read(
            actor, std::string(reviewed.id), backendId);
        if (intent.status != IntentStatus::ok)
            return {CatalogReadStatus::unavailable, {}};

        AddonCatalogRecord record;
        record.moduleId = std::string(reviewed.id);
        record.packageName = std::string(reviewed.packageName);
        record.desiredEnabled = intent.intent.desiredEnabled;
        record.revision = intent.intent.revision;
        // No installed provider is approved or executable at this stage.
        // A persisted administrator wish does not imply active capability.
        record.active = false;
        result.modules.push_back(std::move(record));
    }
    result.status = CatalogReadStatus::ok;
    return result;
}

} // namespace vdrsuite::addons
