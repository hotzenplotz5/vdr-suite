#pragma once

#include "BackendAgentCommand.h"
#include "VdrRecordingCacheRepository.h"

#include <vector>
#include <functional>

#include <cstdint>
#include <string>

class BackendAccessPolicy;
namespace vdrsuite::addons { class AddonActivationIntentRepository; }
class BackendAgentCommandRepository;
class BackendAgentRepository;
class BackendRegistryService;
class SecurityIdentityRepository;
class SecurityPermissionGrantRepository;

namespace vdrsuite::operations {
class MutationOperationRepository;
}

namespace vdrsuite::addons {

// Production Core authorization at the Agent's existing durable RECEIPT
// boundary. This reads the *same* operation, actor/grant and command stores
// used elsewhere in Suite. It never schedules jobs or executes an add-on.
class MediaImportOperationFence {
public:
    MediaImportOperationFence(
        vdrsuite::operations::MutationOperationRepository& operations,
        SecurityIdentityRepository& identities,
        SecurityPermissionGrantRepository& grants,
        BackendAgentRepository& agents,
        BackendAgentCommandRepository& commands,
        BackendRegistryService& backendRegistry,
        BackendAccessPolicy& backendAccess,
        const AddonActivationIntentRepository& activationIntents,
        std::function<bool(const std::string&, std::int64_t)> trustedProviderCheck);

    bool check(const BackendAgentCommandAssignment& assignment,
               const BackendAgentCommandReceipt& receipt,
               std::int64_t now, std::string& reason) const;

    // Used by the existing Agent CommandRepository assignment service.
    // No synthetic receipt or extra queue is created during reservation.
    bool checkCandidate(const BackendAgentCommandAssignment& assignment,
                        std::int64_t now, std::string& reason) const;

    // A7: backend- and actor-specific live discovery. The same authoritative
    // Agent capability, Core actor/grant and audited activation gates as
    // dispatch; no manifest, stored intent or administrator role is enough.
    bool effectiveMediaImportCapability(const std::string& actorId,
                                        const std::string& backendId,
                                        std::int64_t now,
                                        std::string& reason) const;

    // Called only AFTER the existing Agent result repository durably stores
    // the result. Never equate handler exit with a verified VDR recording.
    bool applyResult(const BackendAgentCommandAssignment& assignment,
                     const BackendAgentCommandResult& result,
                     std::int64_t now, std::string& reason) const;

    // Invoked after the existing native-provider refresh successfully commits.
    // No new scheduler, readback database, or daemon-side media filesystem IO.
    // Only a fresh ready cache with one correlated native recording may
    // promote the canonical ADR-0042 operation to succeeded.
    std::size_t reconcileRecordingCache(
        const std::string& backendId,
        const VdrRecordingCacheStatus& cacheStatus,
        const std::vector<VdrRecording>& recordings,
        std::int64_t now) const;

private:
    bool checkCore(const BackendAgentCommandAssignment& assignment,
                   std::int64_t now, bool requireDispatching,
                   std::string& reason) const;
    bool actorBackendAllowed(const std::string& actorId,
                             const std::string& backendId,
                             std::int64_t now, std::string& reason) const;

    vdrsuite::operations::MutationOperationRepository& operations_;
    SecurityIdentityRepository& identities_;
    SecurityPermissionGrantRepository& grants_;
    BackendAgentRepository& agents_;
    BackendAgentCommandRepository& commands_;
    BackendRegistryService& backendRegistry_;
    BackendAccessPolicy& backendAccess_;
    const AddonActivationIntentRepository& activationIntents_;
    std::function<bool(const std::string&, std::int64_t)> trustedProviderCheck_;
};

} // namespace vdrsuite::addons
