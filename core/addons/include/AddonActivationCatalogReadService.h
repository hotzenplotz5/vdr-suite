#pragma once

#include "AddonActivationIntentRepository.h"
#include "AddonCatalog.h"

#include <cstdint>
#include <string>
#include <vector>

namespace vdrsuite::addons {

// Administrative read-only desired-state view, not installed evidence,
// effective capability discovery or an API/Agent endpoint.
enum class CatalogReadStatus { ok, forbidden, unavailable };

struct AddonCatalogRecord {
    std::string moduleId;
    std::string packageName;
    bool desiredEnabled = false;
    std::int64_t revision = 0;
    bool active = false;
};

struct AddonCatalogReadResult {
    CatalogReadStatus status = CatalogReadStatus::unavailable;
    std::vector<AddonCatalogRecord> modules;
};

class AddonActivationCatalogReadService {
public:
    explicit AddonActivationCatalogReadService(
        const AddonActivationIntentRepository& intents) : intents_(intents) {}

    AddonCatalogReadResult read(const RequestSecurityContext& actor,
                               const std::string& backendId) const;

private:
    const AddonActivationIntentRepository& intents_;
};

} // namespace vdrsuite::addons
