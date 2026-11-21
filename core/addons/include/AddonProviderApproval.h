#pragma once

#include "AddonProviderBinding.h"
#include <string_view>

namespace vdrsuite::addons {

// An immutable approval record is supplied by a separately audited Core
// deployment owner, never by addon.json, dpkg's mutable ledger or HTTP.
// This matcher does not authenticate the record itself and grants no runtime
// execution. The deployment owner must validate signatures/ownership.
struct ProviderApproval {
    std::string_view moduleId;
    std::string_view packageName;
    std::string_view backendId;
    std::string_view binarySha256;
    std::string_view version;
    InstallOrigin origin = InstallOrigin::absent;
    std::uint64_t backendGeneration = 0;
    bool independentlyAuthorized = false;
};

enum class ApprovalReason {
    invalidIdentity, unapproved, originMismatch, backendMismatch,
    generationMismatch, digestMismatch, versionMismatch, invalidBinding, disabledProvider
};

struct ApprovalDecision {
    bool approvedForExecution = false;
    ApprovalReason reason = ApprovalReason::unapproved;
    BindingDecision binding;
};

// Kept distinct from the low-level observed binding. Identical hashes in a
// manifest and on disk can never claim independent approval.
inline ApprovalDecision compareApprovedProvider(
    const ProviderApproval& approved, const ProviderBinding& observed)
{
    if (!reviewedHandler(approved.moduleId, approved.packageName) ||
        approved.moduleId != observed.moduleId ||
        approved.packageName != observed.packageName)
        return {false, ApprovalReason::invalidIdentity, {}};
    if (!approved.independentlyAuthorized)
        return {false, ApprovalReason::unapproved, {}};
    if (approved.origin == InstallOrigin::absent ||
        approved.origin != observed.install.origin)
        return {false, ApprovalReason::originMismatch, {}};
    if (approved.backendId != observed.backendId)
        return {false, ApprovalReason::backendMismatch, {}};
    if (approved.backendGeneration == 0 ||
        approved.backendGeneration != observed.backendGeneration ||
        approved.backendGeneration != observed.observedGeneration)
        return {false, ApprovalReason::generationMismatch, {}};
    if (approved.binarySha256 != observed.approvedBinarySha256 ||
        approved.binarySha256 != observed.observedBinarySha256)
        return {false, ApprovalReason::digestMismatch, {}};
    if (approved.version != observed.approvedVersion ||
        approved.version != observed.installedVersion)
        return {false, ApprovalReason::versionMismatch, {}};
    const BindingDecision binding = reviewProviderBinding(observed);
    // Even a matching approval cannot override malformed backend tokens,
    // digest encoding or an independently rejected installation.
    if (binding.reason != BindingReason::deniedProvider)
        return {false, ApprovalReason::invalidBinding, binding};
    // Never upgrade checked evidence to RuntimeEvidence.packageTrusted.
    return {false, ApprovalReason::disabledProvider, binding};
}

} // namespace vdrsuite::addons
