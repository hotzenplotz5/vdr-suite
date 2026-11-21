#pragma once

#include "BackendAgentCommand.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <string>

class BackendAgentCommandRepository;
class BackendAgentRepository;

namespace vdrsuite::operations {
class MutationOperationRepository;
}
namespace vdrsuite::addons {
class MediaImportOperationFence;
}
namespace vdrsuite::agent {

struct BackendAgentMediaImportAssignmentResult {
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

// A6 Core-side reservation. Reuses the one ADR-0042 operation and the
// existing durable Agent command table; never owns jobs or retries itself.
// Called only by trusted Suite code after the protected API has reserved
// a canonical operation (no public Media Import endpoint yet).
class BackendAgentMediaImportAssignmentService {
public:
    BackendAgentMediaImportAssignmentService(
        vdrsuite::operations::MutationOperationRepository& operations,
        BackendAgentCommandRepository& commands,
        BackendAgentRepository& agents,
        vdrsuite::addons::MediaImportOperationFence& fence);

    BackendAgentMediaImportAssignmentResult assign(
        const RequestSecurityContext& systemContext,
        const std::string& operationId, std::int64_t now);

private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    BackendAgentCommandRepository& commands_;
    BackendAgentRepository& agents_;
    vdrsuite::addons::MediaImportOperationFence& fence_;
};

} // namespace vdrsuite::agent
