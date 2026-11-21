#pragma once

#include "BackendAgentCommand.h"
#include "SecurityIdentity.h"

#include <cstdint>
#include <string>

class BackendAgentCommandRepository;
class BackendAgentRepository;
namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::addons { class MediaShrinkOperationFence; }

namespace vdrsuite::agent {
struct BackendAgentMediaShrinkAssignmentResult {
    bool accepted = false;
    bool replayed = false;
    std::string reasonCode;
    BackendAgentCommandAssignment assignment;
};

// Reuses the durable Agent command journal for the existing ADR-0042 HEVC
// Operation. No second worker queue, no import permission fallback and no
// arbitrary file paths. A timed-out/unknown command is NEVER dispatched again.
class BackendAgentMediaShrinkAssignmentService final {
public:
    BackendAgentMediaShrinkAssignmentService(
        vdrsuite::operations::MutationOperationRepository& operations,
        BackendAgentCommandRepository& commands,
        BackendAgentRepository& agents,
        vdrsuite::addons::MediaShrinkOperationFence& fence);
    BackendAgentMediaShrinkAssignmentResult assign(
        const RequestSecurityContext& systemContext,
        const std::string& operationId, std::int64_t now);
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    BackendAgentCommandRepository& commands_;
    BackendAgentRepository& agents_;
    vdrsuite::addons::MediaShrinkOperationFence& fence_;
};
} // namespace vdrsuite::agent
