#pragma once

#include "AddonProviderAdmission.h"

#include <cstdint>
#include <string_view>

namespace vdrsuite::addons {

// Input is produced by future separately reviewed Core-owned providers.
// This type is NOT a deserializer for addon.json or a public API contract.
struct ProviderBinding {
    std::string_view moduleId;
    std::string_view packageName;
    std::string_view backendId;
    std::string_view approvedBinarySha256;
    std::string_view observedBinarySha256;
    std::string_view approvedVersion;
    std::string_view installedVersion;
    std::uint64_t backendGeneration = 0;
    std::uint64_t observedGeneration = 0;
    HandlerInstallFacts install;
};

enum class BindingReason {
    invalidIdentity, invalidBackend, staleBackend,
    missingApproval, binaryMismatch, versionMismatch,
    deniedProvider
};

struct BindingDecision {
    bool accepted = false;
    BindingReason reason = BindingReason::invalidIdentity;
    ProviderDecision provider;
};

// This checks the *shape and agreement* of injected evidence, not who
// approved it. No caller-provided digest, version or generation is proof of
// independent authorization; nothing here creates packageTrusted=true.
inline BindingDecision reviewProviderBinding(const ProviderBinding& binding)
{
    if (!reviewedHandler(binding.moduleId, binding.packageName))
        return {false, BindingReason::invalidIdentity, {}};
    const auto validBackend = [](std::string_view id) {
        if (id.empty() || id.size() > 128 || id == "*") return false;
        for (const char ch : id)
            if (!((ch >= 'a' && ch <= 'z') ||
                  (ch >= 'A' && ch <= 'Z') ||
                  (ch >= '0' && ch <= '9') ||
                  ch == '-' || ch == '_' || ch == '.'))
                return false;
        return true;
    };
    if (!validBackend(binding.backendId))
        return {false, BindingReason::invalidBackend, {}};
    if (binding.backendGeneration == 0 || binding.observedGeneration == 0 ||
        binding.backendGeneration != binding.observedGeneration)
        return {false, BindingReason::staleBackend, {}};

    const auto validSha256 = [](std::string_view value) {
        if (value.size() != 64) return false;
        for (const char ch : value)
            if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
                return false;
        return true;
    };
    if (!validSha256(binding.approvedBinarySha256))
        return {false, BindingReason::missingApproval, {}};
    if (!validSha256(binding.observedBinarySha256) ||
        binding.observedBinarySha256 != binding.approvedBinarySha256)
        return {false, BindingReason::binaryMismatch, {}};
    if (binding.approvedVersion.empty() || binding.installedVersion.empty() ||
        binding.approvedVersion != binding.installedVersion)
        return {false, BindingReason::versionMismatch, {}};

    const ProviderDecision provider =
        reviewProvider(binding.moduleId, binding.packageName, binding.install);
    // Even all-positive evidence remains blocked by the disabled handler and
    // missing actual backend-Agent owner/descriptor lease.
    return {false, BindingReason::deniedProvider, provider};
}

} // namespace vdrsuite::addons
