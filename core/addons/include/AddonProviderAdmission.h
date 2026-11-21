#pragma once

#include "AddonHandlerRegistry.h"
#include <string_view>

namespace vdrsuite::addons {

// Diagnostic facts supplied by separately audited install providers.
// Never interpret a local dpkg ledger or self-reported checksum as authority.
enum class InstallOrigin { absent, manual, debian };
struct HandlerInstallFacts {
    InstallOrigin origin = InstallOrigin::absent;
    bool metadataPresent = false;
    bool integrityMatches = false;
    bool sourceApproved = false;  // independent administrator/release approval
    bool versionCompatible = false;
    bool ownershipSecure = false;
    bool handlerHealthy = false;
};

enum class ProviderReason {
    missingDefinition, disabledDefinition, missingInstall,
    invalidIntegrity, unapprovedSource, insecureOwnership,
    incompatibleVersion, unhealthyHandler, runtimeUnavailable
};

struct ProviderDecision {
    bool available = false;
    bool packageTrusted = false;
    bool handlerRegistered = false;
    bool handlerHealthy = false;
    ProviderReason reason = ProviderReason::missingDefinition;
};

// No caller-selectable executable. Policy remains fail-closed until the
// actual backend-Agent adapter is registered and deployment reviewed.
inline ProviderDecision reviewProvider(
    std::string_view module, std::string_view package,
    const HandlerInstallFacts& facts)
{
    const auto* definition = reviewedHandler(module, package);
    if (!definition) return {false, false, false, false,
                              ProviderReason::missingDefinition};
    if (facts.origin == InstallOrigin::absent || !facts.metadataPresent)
        return {false, false, false, false, ProviderReason::missingInstall};
    if (!facts.integrityMatches)
        return {false, false, false, false, ProviderReason::invalidIntegrity};
    if (!facts.sourceApproved)
        return {false, false, false, false, ProviderReason::unapprovedSource};
    if (!facts.ownershipSecure)
        return {false, false, false, false, ProviderReason::insecureOwnership};
    if (!facts.versionCompatible)
        return {false, false, false, false, ProviderReason::incompatibleVersion};
    if (!facts.handlerHealthy)
        return {false, false, false, false, ProviderReason::unhealthyHandler};
    if (!definition->executable) return {false, false, false, false,
                                         ProviderReason::disabledDefinition};
    // Wiring this to a real runtime needs a backend-local descriptor lease,
    // generation fencing and re-verification at dispatch time.
    return {false, false, false, false, ProviderReason::runtimeUnavailable};
}

} // namespace vdrsuite::addons
