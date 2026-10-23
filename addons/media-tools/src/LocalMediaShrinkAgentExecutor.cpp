#include "LocalMediaShrinkAgentExecutor.h"

#include "MediaShrinkOperationSelection.h"

#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <utility>

namespace vdrsuite::mediatools {
namespace {
bool known(const std::string& type) {
    return type == kMediaShrinkStageCommandType ||
           type == kMediaShrinkPublishCommandType;
}

bool sameCommand(const BackendAgentCommandAssignment& a,
                 const BackendAgentCommandAssignment& b) {
    return a.present && b.present && a.commandId == b.commandId &&
        a.operationId == b.operationId && a.commandType == b.commandType &&
        a.payload == b.payload && a.payloadVersion == b.payloadVersion &&
        a.requestFingerprint == b.requestFingerprint &&
        a.backendId == b.backendId &&
        a.backendGeneration == b.backendGeneration &&
        a.agentId == b.agentId && a.agentInstanceId == b.agentInstanceId &&
        a.jobId == b.jobId && a.attemptId == b.attemptId &&
        a.claimEpoch == b.claimEpoch && a.deadline == b.deadline &&
        a.verificationPolicy == b.verificationPolicy;
}

bool safeRelative(const fs::path& p) {
    if (p.empty() || p.is_absolute() ||
        p.lexically_normal() != p || p.extension() != ".rec" ||
        p.parent_path().empty()) return false;
    return std::none_of(p.begin(), p.end(), [](const fs::path& c) {
        return c == "." || c == "..";
    });
}

bool validAuthority(const LocalMediaShrinkAuthority& a,
                    const BackendAgentCommandAssignment& command,
                    const MediaShrinkAgentSelection& selection) {
    if (a.actorId.empty() || a.actorId.size() > 160 ||
        a.backendId != command.backendId ||
        a.operationId != command.operationId ||
        a.backendGeneration == 0 ||
        a.backendGeneration != command.backendGeneration ||
        !shrinkOpaqueId(a.recordingId, "rec_") ||
        a.sourceFingerprint.empty() || a.sourceFingerprint.size() > 512 ||
        !a.videoRoot.is_absolute() ||
        a.videoRoot.lexically_normal() != a.videoRoot ||
        a.videoRoot.string().size() > 2048 ||
        !safeRelative(a.relativeRecording)) return false;
    if (selection.action == ShrinkRequestAction::stage &&
        a.recordingId != selection.recordingId) return false;
    if (selection.action == ShrinkRequestAction::publish &&
        selection.expectedReviewRevision != 1) return false;
    return true;
}

bool sameAuthority(const LocalMediaShrinkAuthority& a,
                   const LocalMediaShrinkAuthority& b) {
    return a.actorId == b.actorId && a.backendId == b.backendId &&
        a.operationId == b.operationId &&
        a.recordingId == b.recordingId &&
        a.sourceFingerprint == b.sourceFingerprint &&
        a.backendGeneration == b.backendGeneration &&
        a.videoRoot == b.videoRoot &&
        a.relativeRecording == b.relativeRecording;
}

ShrinkOptions options(const MediaShrinkAgentSelection& request) {
    ShrinkOptions settings;
    settings.maxHeight = request.maxHeight;
    settings.nightMode = request.nightMode;
    if (request.encoder == "vaapi") settings.encoder = ShrinkEncoder::vaapi;
    else if (request.encoder == "qsv") settings.encoder = ShrinkEncoder::qsv;
    else if (request.encoder == "nvenc") settings.encoder = ShrinkEncoder::nvenc;
    else settings.encoder = ShrinkEncoder::software;
    return settings;
}
} // namespace

LocalMediaShrinkAgentExecutor::LocalMediaShrinkAgentExecutor(
    LocalMediaShrinkReviewStore& reviews, LiveCapability available,
    CoreAuthorize authorize, RunnerFactory runner)
    : reviews_(reviews), availability_(std::move(available)),
      authorize_(std::move(authorize)), runner_(std::move(runner)) {}

bool LocalMediaShrinkAgentExecutor::available(const std::string& commandType) {
    if (!known(commandType) || !availability_ || !authorize_ || !runner_)
        return false;
    try { return availability_(commandType); }
    catch (...) { return false; }
}

bool LocalMediaShrinkAgentExecutor::authorize(
    const BackendAgentCommandAssignment& assignment,
    const std::string& backendId, std::uint64_t generation) {
    assignment_.reset();
    authority_.reset();
    reviewId_.clear();
    consumed_ = false;
    MediaShrinkAgentSelection selection;
    if (!assignment.present || !known(assignment.commandType) ||
        assignment.backendId != backendId ||
        assignment.backendGeneration != generation ||
        assignment.payloadVersion != 1 || assignment.claimEpoch == 0 ||
        assignment.commandId.empty() || assignment.operationId.empty() ||
        assignment.requestFingerprint.empty() ||
        assignment.agentId.empty() || assignment.agentInstanceId.empty() ||
        assignment.jobId.empty() || assignment.attemptId.empty() ||
        assignment.deadline <= static_cast<std::int64_t>(std::time(nullptr)) ||
        !parseMediaShrinkAgentSelection(
            assignment.commandType, assignment.payload, selection) ||
        !available(assignment.commandType)) return false;
    if (assignment.verificationPolicy != (
            selection.action == ShrinkRequestAction::stage ?
                "readback_required" : "reconciliation_required"))
        return false;
    try {
        auto evidence = authorize_(assignment);
        if (!evidence || !validAuthority(*evidence, assignment, selection))
            return false;
        if (selection.action == ShrinkRequestAction::publish) {
            const auto prior = reviews_.find(selection.reviewId);
            if (!prior || prior->actorId != evidence->actorId ||
                prior->backendId != evidence->backendId ||
                prior->backendGeneration != evidence->backendGeneration ||
                prior->recordingId != evidence->recordingId ||
                prior->sourceFingerprint != evidence->sourceFingerprint ||
                prior->videoRoot != evidence->videoRoot ||
                prior->relativeRecording != evidence->relativeRecording)
                return false;
        }
        assignment_ = assignment;
        authority_ = std::move(evidence);
        return true;
    } catch (...) { return false; }
}

MediaShrinkAgentOutcome LocalMediaShrinkAgentExecutor::execute(
    const BackendAgentCommandAssignment& assignment) {
    if (consumed_ || !assignment_ || !authority_ ||
        !sameCommand(*assignment_, assignment))
        return MediaShrinkAgentOutcome::rejected;
    consumed_ = true; // Never execute even a failed/unknown assignment twice.
    reviewId_.clear();
    try {
        MediaShrinkAgentSelection selection;
        if (!parseMediaShrinkAgentSelection(
                assignment.commandType, assignment.payload, selection) ||
            !available(assignment.commandType))
            return MediaShrinkAgentOutcome::rejected;
        // Authorization is re-evaluated at the actual mutation boundary.
        const auto fresh = authorize_(assignment);
        if (!fresh || !sameAuthority(*fresh, *authority_) ||
            !validAuthority(*fresh, assignment, selection))
            return MediaShrinkAgentOutcome::rejected;
        auto& runner = runner_();
        if (selection.action == ShrinkRequestAction::stage) {
            const auto plan = inspectShrink(
                fresh->videoRoot, fresh->relativeRecording, options(selection));
            const auto candidate = stageShrink(plan, runner, requireShrinkCapacity);
            LocalMediaShrinkReview local;
            local.actorId = fresh->actorId;
            local.backendId = fresh->backendId;
            local.recordingId = fresh->recordingId;
            local.stageOperationId = fresh->operationId;
            local.sourceFingerprint = fresh->sourceFingerprint;
            local.backendGeneration = fresh->backendGeneration;
            local.videoRoot = fresh->videoRoot;
            local.relativeRecording = fresh->relativeRecording;
            local.candidateDirectory = candidate;
            reviewId_ = reviews_.registerCandidate(local);
        } else {
            const auto local = reviews_.find(selection.reviewId);
            if (!local || local->actorId != fresh->actorId ||
                local->backendId != fresh->backendId ||
                local->backendGeneration != fresh->backendGeneration ||
                local->recordingId != fresh->recordingId ||
                local->sourceFingerprint != fresh->sourceFingerprint ||
                local->videoRoot != fresh->videoRoot ||
                local->relativeRecording != fresh->relativeRecording)
                return MediaShrinkAgentOutcome::rejected;
            // A durable irreversible start fence prevents double publication
            // after any crash, timeout or uncertain Agent result.
            if (!reviews_.claimOnce(selection.reviewId, fresh->operationId))
                return MediaShrinkAgentOutcome::unknown;
            const auto plan = inspectShrink(local->videoRoot,
                                             local->relativeRecording);
            (void)publishShrinkCandidate(plan, local->candidateDirectory, runner);
            reviewId_ = selection.reviewId;
        }
        return MediaShrinkAgentOutcome::submitted;
    } catch (...) {
        // Side effects may already exist; Core readback must disambiguate.
        reviewId_.clear();
        return MediaShrinkAgentOutcome::unknown;
    }
}

std::string LocalMediaShrinkAgentExecutor::takeReviewId() {
    return std::exchange(reviewId_, {});
}
} // namespace vdrsuite::mediatools
