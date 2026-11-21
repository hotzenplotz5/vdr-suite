#pragma once

#include "AddonProviderBinding.h"
#include "AddonAccessPolicy.h"
#include <string>

namespace vdrsuite::addons {

// Read-only adapter for the existing admission contract. Never accept an
// externally constructed RuntimeEvidence as proof that a provider is trusted.
struct ProviderReadResult {
    RuntimeEvidence evidence;
    BindingDecision binding;
};

inline ProviderReadResult readUntrustedProvider(const ProviderBinding& candidate)
{
    ProviderReadResult result;
    result.binding = reviewProviderBinding(candidate);
    // No public API receives these diagnostic details. The only recognized
    // module/package pair comes from immutable compiled Core definitions.
    const auto* definition =
        reviewedHandler(candidate.moduleId, candidate.packageName);
    if (definition) {
        result.evidence.moduleId = std::string(definition->moduleId);
        result.evidence.packageName = std::string(definition->packageName);
    }
    // Backend ID alone confers no authority. Generation validation must
    // happen again at the operation's backend-Agent dispatch boundary.
    if (result.binding.reason == BindingReason::deniedProvider)
        result.evidence.backendId = std::string(candidate.backendId);
    // installed/manifestValid and compatible version are observational, not
    // executable; do not mark them as verified until a real provider exists.
    // packageTrusted, handlerRegistered, handlerHealthy and administratorEnabled
    // remain false even when the caller supplies entirely positive facts.
    return result;
}

} // namespace vdrsuite::addons
