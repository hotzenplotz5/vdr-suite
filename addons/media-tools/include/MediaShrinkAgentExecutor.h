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

    // Bounded path-free code for this ONE execution. Never forward exception
    // text, filesystem paths or raw ffmpeg stderr to the Core ledger.
    virtual std::string takeFailureCode() { return {}; }
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

/*
 * A durable Agent result can say "starting/outcome_unknown" with
 * errorCategory="none": the executor completed its private work but its
 * precise review ID was lost at the control-plane boundary. The older
 * "executor_unknown" variant is also supported. Neither is a license to
 * restart work; both only allow Core's strict, independently verified
 * private candidate readback by the ORIGINAL operation identity.
 */
inline bool shrinkUnknownStageAllowsPrivateReadback(
    const BackendAgentCommandResult& result)
{
    return result.dispatchState == "starting" &&
        result.verificationState == "outcome_unknown" &&
        result.resultCategory == "outcome_unknown" &&
        result.retryClassification == "reconcile_only" &&
        (result.errorCategory == "none" ||
         result.errorCategory == "executor_unknown") &&
        result.resultEvidence.empty();
}

} // namespace vdrsuite::mediatools
