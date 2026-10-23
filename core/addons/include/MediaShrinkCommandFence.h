#pragma once

#include "MediaShrinkAgentCommand.h"
#include "BackendAgentCommand.h"
#include "MutationOperationRepository.h"

#include <cstdint>
#include <string>

namespace vdrsuite::addons {

// Structural, non-authorizing admission to the canonical Core/Agent pipeline.
// This check alone NEVER permits execution or capability advertisement.
enum class MediaShrinkCommandFenceResult {
    candidateOnly,
    invalidAssignment,
    staleAuthority,
    operationMismatch,
    payloadMismatch
};

inline MediaShrinkCommandFenceResult inspectMediaShrinkCommandFence(
    const vdrsuite::operations::MutationOperation& operation,
    const vdrsuite::operations::MutationOperationPayload& durablePayload,
    const BackendAgentCommandAssignment& assignment,
    std::int64_t now)
{
    using namespace vdrsuite::operations;
    using Decision = MediaShrinkCommandFenceResult;
    if (!assignment.present || assignment.protocolVersion != "vdr-suite-agent/1" ||
        assignment.commandId.empty() || assignment.operationId.empty() ||
        assignment.jobId.empty() || assignment.attemptId.empty() ||
        assignment.agentId.empty() || assignment.agentInstanceId.empty() ||
        assignment.requestFingerprint.empty() || assignment.claimEpoch == 0 ||
        assignment.payloadVersion != 1 ||
        (assignment.commandType !=
            vdrsuite::mediatools::kMediaShrinkStageCommandType &&
         assignment.commandType !=
            vdrsuite::mediatools::kMediaShrinkPublishCommandType))
        return Decision::invalidAssignment;
    if (now <= 0 || operation.backendGeneration == 0 ||
        operation.backendGeneration != assignment.backendGeneration ||
        assignment.deadline <= now || operation.deadline < assignment.deadline ||
        operation.deadline <= now || operation.requestedAt > assignment.assignedAt ||
        assignment.assignedAt > now)
        return Decision::staleAuthority;
    if (operation.state != MutationOperationState::dispatching ||
        operation.operationId != assignment.operationId ||
        operation.backendId != assignment.backendId ||
        operation.backendId.empty() || operation.backendId == "*" ||
        operation.actorId.empty() || operation.requestFingerprint.empty() ||
        !operation.resultReference.empty())
        return Decision::operationMismatch;
    if (durablePayload.operationId != operation.operationId ||
        durablePayload.payloadVersion != 1 ||
        durablePayload.payloadType != assignment.commandType ||
        durablePayload.payload != assignment.payload ||
        durablePayload.payloadFingerprint != operation.requestFingerprint ||
        durablePayload.payloadFingerprint.empty())
        return Decision::payloadMismatch;

    vdrsuite::mediatools::MediaShrinkAgentSelection selected;
    if (!vdrsuite::mediatools::parseMediaShrinkAgentSelection(
            assignment.commandType, assignment.payload, selected))
        return Decision::payloadMismatch;
    const bool stage = selected.action ==
        vdrsuite::mediatools::ShrinkRequestAction::stage;
    if (operation.actionFamily != (stage ? "media.shrink.stage" :
                                            "media.shrink.publish") ||
        operation.resourceType != (stage ? "Recording" : "MediaShrinkReview") ||
        operation.resourceId != (stage ? selected.recordingId :
                                         selected.reviewId) ||
        operation.expectedResourceFingerprint.empty() ||
        operation.verificationPolicy !=
            (stage ? MutationOperationVerificationPolicy::readbackRequired :
                     MutationOperationVerificationPolicy::reconciliationRequired) ||
        assignment.verificationPolicy !=
            (stage ? "readback_required" : "reconciliation_required") ||
        (stage ? operation.expectedRevision.empty() :
                 operation.expectedRevision !=
                     std::to_string(selected.expectedReviewRevision)))
        return Decision::operationMismatch;

    return Decision::candidateOnly;
}

} // namespace vdrsuite::addons
