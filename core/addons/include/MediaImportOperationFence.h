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
        BackendAccessPolicy& backendAccess);

    bool check(const BackendAgentCommandAssignment& assignment,
               const BackendAgentCommandReceipt& receipt,
               std::int64_t now, std::string& reason) const;

    // Used by the existing Agent CommandRepository assignment service.
    // No synthetic receipt or extra queue is created during reservation.
    bool checkCandidate(const BackendAgentCommandAssignment& assignment,
                        std::int64_t now, std::string& reason) const;

    // Called only AFTER the existing Agent result repository durably stores
    // the result. Never equate handler exit with a verified VDR recording.
    bool applyResult(const BackendAgentCommandAssignment& assignment,
                     const BackendAgentCommandResult& result,
                     std::int64_t now, std::string& reason) const;

private:
    bool checkCore(const BackendAgentCommandAssignment& assignment,
                   std::int64_t now, bool requireDispatching,
                   std::string& reason) const;

    vdrsuite::operations::MutationOperationRepository& operations_;
    SecurityIdentityRepository& identities_;
    SecurityPermissionGrantRepository& grants_;
    BackendAgentRepository& agents_;
    BackendAgentCommandRepository& commands_;
    BackendRegistryService& backendRegistry_;
    BackendAccessPolicy& backendAccess_;
};

} // namespace vdrsuite::addons
