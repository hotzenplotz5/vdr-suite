#pragma once

#include "AddonAccessPolicy.h"
#include "AddonActivationIntentRepository.h"

#include <cstdint>

namespace vdrsuite::addons {

// Diagnostic only: NOT an executable permission, installed provider status,
// backend lease, public API or dispatch token.
struct AdmissionPreview {
    AccessDecision policy;
    std::int64_t observedIntentRevision = 0;
    bool executable = false; // cannot become true in this slice
};

class AddonAdmissionReadService {
public:
    explicit AddonAdmissionReadService(const AddonActivationIntentRepository& intents)
        : intents_(intents) {}

    AdmissionPreview previewMediaImport(
        const RequestSecurityContext& actor,
        const BackendAccessDecision& backend,
        const RuntimeEvidence& candidateEvidence) const;

private:
    const AddonActivationIntentRepository& intents_;
};

} // namespace vdrsuite::addons
