#include "AddonProviderReadService.h"
#include <cassert>
#include <string>

int main()
{
    using namespace vdrsuite::addons;
    const std::string digest(64, 'a');
    HandlerInstallFacts facts;
    facts.metadataPresent = true;
    facts.integrityMatches = true;
    facts.sourceApproved = true;
    facts.versionCompatible = true;
    facts.ownershipSecure = true;
    facts.handlerHealthy = true;
    for (const auto origin : {InstallOrigin::manual, InstallOrigin::debian}) {
        facts.origin = origin;
        const ProviderBinding binding{
            "rectools", "vdr-suite-addon-media-tools", "backend-1",
            digest, digest, "0.1.0", "0.1.0", 42, 42, facts
        };
        const auto result = readUntrustedProvider(binding);
        assert(!result.binding.accepted);
        assert(!result.evidence.installed);
        assert(!result.evidence.manifestValid);
        assert(!result.evidence.packageTrusted);
        assert(!result.evidence.versionCompatible);
        assert(!result.evidence.administratorEnabled);
        assert(!result.evidence.handlerRegistered);
        assert(!result.evidence.handlerHealthy);
        assert(result.evidence.moduleId == "rectools");
        assert(result.evidence.backendId == "backend-1");
    }
    const ProviderBinding wrong{
        "image", "vdr-suite-addon-image", "backend-1",
        digest, digest, "0.1.0", "0.1.0", 42, 42, facts
    };
    const auto rejected = readUntrustedProvider(wrong);
    assert(rejected.evidence.moduleId.empty());
    assert(rejected.evidence.backendId.empty());
    assert(!rejected.evidence.packageTrusted);
}
