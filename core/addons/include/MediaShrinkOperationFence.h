#pragma once

#include "BackendAgentCommand.h"
#include <cstdint>
#include <string>

class BackendAccessPolicy;
class BackendAgentCommandRepository;
class BackendAgentRepository;
class BackendRegistryService;
class SecurityIdentityRepository;
class SecurityPermissionGrantRepository;

namespace vdrsuite::operations { class MutationOperationRepository; }

namespace vdrsuite::addons {
class AddonActivationIntentRepository;
class MediaShrinkReviewRepository;

// Separate Core authorization for each HEVC mutation. The command parser,
// manifest and local executor are never authority. This service reuses the
// existing actor/grant/agent/operation stores at the durable receipt boundary.
class MediaShrinkOperationFence final {
public:
    MediaShrinkOperationFence(
        vdrsuite::operations::MutationOperationRepository& operations,
        SecurityIdentityRepository& identities,
        SecurityPermissionGrantRepository& grants,
        BackendAgentRepository& agents,
        BackendAgentCommandRepository& commands,
        BackendRegistryService& backendRegistry,
        BackendAccessPolicy& backendAccess,
        const AddonActivationIntentRepository& activationIntents,
        MediaShrinkReviewRepository& reviews);

    bool check(const BackendAgentCommandAssignment& assignment,
               const BackendAgentCommandReceipt& receipt,
               std::int64_t now, std::string& reason) const;

    // Non-authorizing structural preflight is in MediaShrinkCommandFence.h;
    // no direct worker execution is exposed from this class.
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    SecurityIdentityRepository& identities_;
    SecurityPermissionGrantRepository& grants_;
    BackendAgentRepository& agents_;
    BackendAgentCommandRepository& commands_;
    BackendRegistryService& backendRegistry_;
    BackendAccessPolicy& backendAccess_;
    const AddonActivationIntentRepository& activationIntents_;
    MediaShrinkReviewRepository& reviews_;
};
} // namespace vdrsuite::addons
