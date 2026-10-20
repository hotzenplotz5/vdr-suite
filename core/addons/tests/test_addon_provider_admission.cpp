#include "AddonProviderAdmission.h"
#include <cassert>

int main()
{
    using namespace vdrsuite::addons;
    HandlerInstallFacts evidence;
    evidence.metadataPresent = true;
    evidence.integrityMatches = true;
    evidence.sourceApproved = true;
    evidence.versionCompatible = true;
    evidence.ownershipSecure = true;
    evidence.handlerHealthy = true;
    for (auto origin : {InstallOrigin::manual, InstallOrigin::debian}) {
        evidence.origin = origin;
        auto result = reviewProvider("rectools", "vdr-suite-addon-media-tools", evidence);
        assert(!result.available && !result.packageTrusted);
        assert(!result.handlerRegistered && !result.handlerHealthy);
        assert(result.reason == ProviderReason::disabledDefinition);
    }
    assert(reviewProvider("rectools", "vdr-suite-addon-music", evidence).reason ==
           ProviderReason::missingDefinition);
    assert(reviewProvider("image", "vdr-suite-addon-image", evidence).reason ==
           ProviderReason::missingDefinition);
    assert(reviewProvider("music", "vdr-suite-addon-music", evidence).reason ==
           ProviderReason::missingDefinition);
    evidence.sourceApproved = false;
    assert(!reviewProvider("rectools", "vdr-suite-addon-media-tools", evidence).packageTrusted);
}
