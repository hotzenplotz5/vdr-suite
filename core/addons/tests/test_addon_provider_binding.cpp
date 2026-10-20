#include "AddonProviderBinding.h"
#include <cassert>
#include <string>

int main()
{
    using namespace vdrsuite::addons;
    const std::string digest(64, 'a');
    HandlerInstallFacts facts;
    facts.origin = InstallOrigin::manual;
    facts.metadataPresent = true;
    facts.integrityMatches = true;
    facts.sourceApproved = true;
    facts.versionCompatible = true;
    facts.ownershipSecure = true;
    facts.handlerHealthy = true;
    ProviderBinding binding{
        "rectools", "vdr-suite-addon-media-tools", "backend-1",
        digest, digest, "0.1.0", "0.1.0", 8, 8, facts
    };
    auto reason = [&]() { return reviewProviderBinding(binding).reason; };
    assert(reason() == BindingReason::deniedProvider);
    auto result = reviewProviderBinding(binding);
    assert(!result.accepted && !result.provider.available);
    assert(!result.provider.packageTrusted && !result.provider.handlerRegistered);

    for (const auto origin : {InstallOrigin::manual, InstallOrigin::debian}) {
        binding.install.origin = origin;
        assert(reason() == BindingReason::deniedProvider);
        binding.observedGeneration = 9;
        assert(reason() == BindingReason::staleBackend);
        binding.observedGeneration = 8;
        binding.backendGeneration = 0;
        assert(reason() == BindingReason::staleBackend);
        binding.backendGeneration = 8;
        binding.backendId = "*";
        assert(reason() == BindingReason::invalidBackend);
        binding.backendId = "backend-1";
        binding.observedBinarySha256 = std::string(64, 'b');
        assert(reason() == BindingReason::binaryMismatch);
        binding.observedBinarySha256 = digest;
        binding.approvedBinarySha256 = "not-approved";
        assert(reason() == BindingReason::missingApproval);
        binding.approvedBinarySha256 = digest;
        binding.installedVersion = "0.2.0";
        assert(reason() == BindingReason::versionMismatch);
        binding.installedVersion = "0.1.0";
        binding.install.sourceApproved = false;
        assert(reason() == BindingReason::deniedProvider);
        assert(!reviewProviderBinding(binding).provider.packageTrusted);
        binding.install.sourceApproved = true;
    }

    binding.moduleId = "image";
    binding.packageName = "vdr-suite-addon-image";
    assert(reason() == BindingReason::invalidIdentity);
    binding.moduleId = "music";
    binding.packageName = "vdr-suite-addon-music";
    assert(reason() == BindingReason::invalidIdentity);
    binding.moduleId = "rectools";
    binding.packageName = "vdr-suite-addon-media-tools";
    binding.approvedBinarySha256 = std::string(64, 'A');
    assert(reason() == BindingReason::missingApproval);
}
