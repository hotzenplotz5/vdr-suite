#pragma once

#include "AddonActivationIntentRepository.h"
#include "MutationOperationRepository.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace vdrsuite::addons {

// Point-in-time, read-only diagnostic for one exact backend's Media Import
// state. NEVER an install/uninstall authorization or an exclusive drain lease.
// An actual package maintenance owner must separately stop new admissions,
// synchronize in-flight Agent execution, validate trusted package origin and
// re-read these authorities under its own coordinated maintenance fence.
enum class MaintenanceReadReason {
    forbidden,
    activationUnavailable,
    activationNotExplicitlyDisabled,
    operationsUnavailable,
    scanSaturated,
    unresolvedMediaImports,
    locallyQuiescentSnapshot,
};

struct MaintenanceReadResult {
    bool locallyQuiescent = false;
    MaintenanceReadReason reason = MaintenanceReadReason::forbidden;
    std::int64_t observedIntentRevision = 0;
    std::size_t unresolvedOperations = 0;
};

class AddonMediaToolsMaintenanceReadService {
public:
    AddonMediaToolsMaintenanceReadService(
        AddonActivationIntentRepository& intents,
        vdrsuite::operations::MutationOperationRepository& operations)
        : intents_(intents), operations_(operations) {}

    MaintenanceReadResult inspect(const RequestSecurityContext& actor,
                                  const std::string& backendId) const;

private:
    AddonActivationIntentRepository& intents_;
    vdrsuite::operations::MutationOperationRepository& operations_;
};

} // namespace vdrsuite::addons
