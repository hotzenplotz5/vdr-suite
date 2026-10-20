#include "AddonProviderApproval.h"
#include <cassert>
#include <initializer_list>
#include <string>

int main()
{
    using namespace vdrsuite::addons;
    const std::string digest(64, 'a'), wrongDigest(64, 'b');
    HandlerInstallFacts facts{};
    facts.metadataPresent = facts.integrityMatches = facts.sourceApproved =
        facts.versionCompatible = facts.ownershipSecure = facts.handlerHealthy = true;
    for (auto origin : {InstallOrigin::manual, InstallOrigin::debian}) {
        facts.origin = origin;
        ProviderBinding observed{"rectools", "vdr-suite-addon-media-tools",
            "backend-a", digest, digest, "0.1.0", "0.1.0", 12, 12, facts};
        ProviderApproval approval{"rectools", "vdr-suite-addon-media-tools",
            "backend-a", digest, "0.1.0", origin, 12, true};
        auto review = [&] { return compareApprovedProvider(approval, observed); };
        assert(review().reason == ApprovalReason::disabledProvider);
        assert(!review().approvedForExecution && !review().binding.accepted);
        approval.independentlyAuthorized = false;
        assert(review().reason == ApprovalReason::unapproved);
        approval.independentlyAuthorized = true;
        approval.origin = InstallOrigin::absent;
        assert(review().reason == ApprovalReason::originMismatch);
        approval.origin = origin;
        approval.backendId = "other";
        assert(review().reason == ApprovalReason::backendMismatch);
        approval.backendId = "backend-a";
        approval.backendGeneration = 11;
        assert(review().reason == ApprovalReason::generationMismatch);
        approval.backendGeneration = 12;
        approval.binarySha256 = wrongDigest;
        assert(review().reason == ApprovalReason::digestMismatch);
        approval.binarySha256 = digest;
        approval.version = "0.2.0";
        assert(review().reason == ApprovalReason::versionMismatch);
        approval.version = "0.1.0";
        observed.packageName = "vdr-suite-addon-music";
        assert(review().reason == ApprovalReason::invalidIdentity);
    }
}
