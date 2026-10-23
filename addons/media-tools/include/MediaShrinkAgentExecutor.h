#pragma once

#include "BackendAgentCommand.h"
#include "MediaShrinkAgentCommand.h"

#include <cstdint>
#include <string>

namespace vdrsuite::mediatools {

// A reviewed backend-local composition MAY inject an implementation.
// The shipped Agent injects none. Manifest fields and COMMAND_TYPES never
// create this authority, and successful submission is NOT VDR readback.
enum class MediaShrinkAgentOutcome {
    rejected,   // Executor proves no side effect.
    submitted,  // Staging/publication attempted; requires Core verification.
    unknown     // Side-effect ambiguity: reconciliation only, no blind retry.
};

class IMediaShrinkAgentExecutor {
public:
    virtual ~IMediaShrinkAgentExecutor() = default;

    // Re-evaluate trusted installation, backend/generation and the exact
    // stage/publish permission. The two commands have separate admission.
    virtual bool available(const std::string& commandType) = 0;
    virtual bool authorize(const BackendAgentCommandAssignment& assignment,
                           const std::string& backendId,
                           std::uint64_t generation) = 0;
    virtual MediaShrinkAgentOutcome execute(
        const BackendAgentCommandAssignment& assignment) = 0;

    // One-shot opaque stage review ID, or the already-bound publish review
    // ID. Never expose a local filesystem path in a control-plane result.
    virtual std::string takeReviewId() { return {}; }
};

inline bool validShrinkReviewEvidence(
    const BackendAgentCommandAssignment& assignment,
    const std::string& reviewId)
{
    MediaShrinkAgentSelection selection;
    if (!shrinkOpaqueId(reviewId, "shk_") ||
        !parseMediaShrinkAgentSelection(
            assignment.commandType, assignment.payload, selection))
        return false;
    return selection.action == ShrinkRequestAction::stage ||
           (selection.action == ShrinkRequestAction::publish &&
            selection.reviewId == reviewId);
}

} // namespace vdrsuite::mediatools
