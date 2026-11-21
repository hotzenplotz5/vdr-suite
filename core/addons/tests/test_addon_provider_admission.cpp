#include "AddonProviderAdmission.h"
#include <cassert>
#include <initializer_list>

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
    // Both install pathways must reject each missing independent prerequisite.
    for (auto origin : {InstallOrigin::manual, InstallOrigin::debian}) {
        evidence.origin = origin;
        const auto reason = [&]() {
            return reviewProvider("rectools", "vdr-suite-addon-media-tools", evidence).reason;
        };
        evidence.metadataPresent = false;
        assert(reason() == ProviderReason::missingInstall);
        evidence.metadataPresent = true;
        evidence.integrityMatches = false;
        assert(reason() == ProviderReason::invalidIntegrity);
        evidence.integrityMatches = true;
        evidence.sourceApproved = false;
        assert(reason() == ProviderReason::unapprovedSource);
        evidence.sourceApproved = true;
        evidence.ownershipSecure = false;
        assert(reason() == ProviderReason::insecureOwnership);
        evidence.ownershipSecure = true;
        evidence.versionCompatible = false;
        assert(reason() == ProviderReason::incompatibleVersion);
        evidence.versionCompatible = true;
        evidence.handlerHealthy = false;
        assert(reason() == ProviderReason::unhealthyHandler);
        evidence.handlerHealthy = true;
        assert(reason() == ProviderReason::disabledDefinition);
    }
    evidence.origin = InstallOrigin::absent;
    assert(reviewProvider("rectools", "vdr-suite-addon-media-tools", evidence).reason ==
           ProviderReason::missingInstall);
}
