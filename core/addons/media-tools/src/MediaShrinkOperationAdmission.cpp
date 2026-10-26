#include "MediaShrinkOperationAdmission.h"

#include "MediaShrinkAgentCommand.h"
#include "MediaShrinkOperationSelection.h"
#include "MediaShrinkReviewRepository.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::addons {
namespace {
using namespace vdrsuite::operations;
using Status = MediaShrinkAdmissionStatus;
using Result = MediaShrinkAdmissionResult;
using Selection = vdrsuite::mediatools::MediaShrinkAgentSelection;

Result out(Status status, const MutationOperation& operation = {}) {
    return {status, operation};
}
bool identity(const std::string& value) {
    return !value.empty() && value.size() <= 128 &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') ||
                   (c >= 'A' && c <= 'Z') || c == '_' || c == '-' ||
                   c == '.' || c == ':';
        });
}
bool idempotency(const std::string& value) {
    return value.size() >= 8 && value.size() <= 128 && identity(value);
}
bool safeNow(std::int64_t now) {
    return now > 0 &&
           now < std::numeric_limits<std::int64_t>::max() - 14400;
}
bool fingerprint(const std::string& text) {
    return identity(text);
}
std::string stagePayload(const MediaShrinkStageAdmission& request) {
    return "{\"recordingId\":\"" + request.recordingId +
        "\",\"encoder\":\"" + request.encoder +
        "\",\"maxHeight\":" + std::to_string(request.maxHeight) +
        ",\"nightMode\":" + (request.nightMode ? "true" : "false") + "}";
}
std::string publishPayload(const std::string& id, std::uint64_t revision) {
    return "{\"reviewId\":\"" + id + "\",\"revision\":" +
        std::to_string(revision) + "}";
}
std::string actionFingerprint(const std::string& type,
                              const std::string& backend,
                              const std::string& resource,
                              const std::string& source,
                              const std::string& serialized) {
    // The source fingerprint is trusted Core readback, not supplied by HTTP.
    // Length prefixes prevent combinations of fields aliasing each other.
    return "shrink-api/1|" + type + "|" + std::to_string(backend.size()) +
        ":" + backend + "|" + resource + "|" + source + "|" + serialized;
}
MutationOperation newOperation(
    const std::string& actor, const std::string& backend,
    const std::string& key, std::uint64_t generation,
    const std::string& resourceType, const std::string& resourceId,
    const std::string& action, const std::string& revision,
    const std::string& sourceFingerprint,
    const std::string& requestFingerprint,
    MutationOperationVerificationPolicy verification, std::int64_t now)
{
    MutationOperation op;
    op.operationId = generateMutationOperationId();
    op.idempotencyKey = key;
    op.actorId = actor;
    op.backendId = backend;
    op.backendGeneration = generation;
    op.resourceType = resourceType;
    op.resourceId = resourceId;
    op.expectedRevision = revision;
    op.expectedResourceFingerprint = sourceFingerprint;
    op.actionFamily = action;
    op.requestFingerprint = requestFingerprint;
    op.requestedAt = now;
    // Full-length transcoding must not be constrained to the short
    // media-import receipt deadline. The Agent still has its own crash fence.
    op.deadline = now + 14400;
    op.verificationPolicy = verification;
    op.state = MutationOperationState::accepted;
    op.updatedAt = now;
    return op;
}
MutationOperationPayload newPayload(const MutationOperation& op,
                                    const std::string& type,
                                    const std::string& serialized) {
    MutationOperationPayload payload;
    payload.operationId = op.operationId;
    payload.payloadType = type;
    payload.payloadVersion = 1;
    payload.payload = serialized;
    payload.payloadFingerprint = "shrink-payload/1|" + serialized;
    return payload;
}
bool replayMatches(MutationOperationRepository& repo,
                   const MutationOperation& previous,
                   const MutationOperation& wanted,
                   const std::string& type,
                   const std::string& serialized) {
    if (previous.idempotencyKey != wanted.idempotencyKey ||
        previous.actorId != wanted.actorId ||
        previous.backendId != wanted.backendId ||
        previous.backendGeneration != wanted.backendGeneration ||
        previous.resourceType != wanted.resourceType ||
        previous.resourceId != wanted.resourceId ||
        previous.expectedRevision != wanted.expectedRevision ||
        previous.expectedResourceFingerprint != wanted.expectedResourceFingerprint ||
        previous.actionFamily != wanted.actionFamily ||
        previous.requestFingerprint != wanted.requestFingerprint ||
        previous.verificationPolicy != wanted.verificationPolicy)
        return false;
    const auto stored = repo.findPayloadByOperationId(previous.operationId);
    return stored.ok() && stored.payload.payloadVersion == 1 &&
           stored.payload.payloadType == type &&
           stored.payload.payload == serialized &&
           stored.payload.payloadFingerprint == "shrink-payload/1|" + serialized;
}
Result reserve(MutationOperationRepository& operations,
               const MutationOperation& proposed,
               const std::string& type, const std::string& serialized) {
    const auto previous = operations.findByIdempotencyScope(
        proposed.actorId, proposed.backendId, proposed.resourceType,
        proposed.resourceId, proposed.actionFamily, proposed.idempotencyKey);
    if (previous.ok())
        return replayMatches(operations, previous.operation, proposed,
                             type, serialized)
             ? out(Status::replayed, previous.operation)
             : out(Status::idempotencyConflict);
    if (previous.status != MutationOperationRepositoryStatus::notFound)
        return out(Status::unavailable);
    const auto result = operations.reserveWithPayload(
        proposed, newPayload(proposed, type, serialized));
    if (result.ok()) return out(Status::accepted, result.operation);
    if (result.status == MutationOperationRepositoryStatus::idempotencyConflict) {
        const auto raced = operations.findByIdempotencyScope(
            proposed.actorId, proposed.backendId, proposed.resourceType,
            proposed.resourceId, proposed.actionFamily, proposed.idempotencyKey);
        if (raced.ok() && replayMatches(
                operations, raced.operation, proposed, type, serialized))
            return out(Status::replayed, raced.operation);
        return out(Status::idempotencyConflict);
    }
    if (result.status == MutationOperationRepositoryStatus::invalid)
        return out(Status::invalid);
    if (result.status == MutationOperationRepositoryStatus::operationConflict)
        return out(Status::reviewConflict);
    return out(Status::unavailable);
}
} // namespace

MediaShrinkOperationAdmissionService::MediaShrinkOperationAdmissionService(
    MutationOperationRepository& operations,
    MediaShrinkReviewRepository& reviews,
    IMediaShrinkAdmissionAuthority& authority)
    : operations_(operations), reviews_(reviews), authority_(authority) {}

MediaShrinkAdmissionResult MediaShrinkOperationAdmissionService::stage(
    const MediaShrinkStageAdmission& req)
{
    if (!safeNow(req.now) || !identity(req.actorId) ||
        !identity(req.backendId) || req.backendId == "*" ||
        !idempotency(req.idempotencyKey) ||
        !fingerprint(req.sourceFingerprint))
        return out(Status::invalid);
    const auto serialized = stagePayload(req);
    Selection selection;
    if (!vdrsuite::mediatools::parseMediaShrinkAgentSelection(
            vdrsuite::mediatools::kMediaShrinkStageCommandType,
            serialized, selection))
        return out(Status::invalid);
    std::uint64_t generation = 0;
    if (!authority_.allowStage(req.actorId, req.backendId, req.recordingId,
                               req.sourceFingerprint, req.now, generation) ||
        generation == 0 ||
        generation > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
        return out(Status::forbidden);
    const std::string requestFingerprint = actionFingerprint(
        "stage", req.backendId, req.recordingId,
        req.sourceFingerprint, serialized);
    if (requestFingerprint.size() > 512) return out(Status::invalid);
    const auto proposed = newOperation(
        req.actorId, req.backendId, req.idempotencyKey, generation,
        "Recording", req.recordingId, "media.shrink.stage", "source-v1",
        req.sourceFingerprint, requestFingerprint,
        MutationOperationVerificationPolicy::readbackRequired, req.now);
    if (!mutationOperationIdCanonical(proposed.operationId))
        return out(Status::unavailable);
    return reserve(operations_, proposed,
        vdrsuite::mediatools::kMediaShrinkStageCommandType, serialized);
}

MediaShrinkAdmissionResult MediaShrinkOperationAdmissionService::publish(
    const MediaShrinkPublishAdmission& req)
{
    if (!safeNow(req.now) || !identity(req.actorId) ||
        !identity(req.backendId) || req.backendId == "*" ||
        !idempotency(req.idempotencyKey) ||
        !vdrsuite::mediatools::shrinkOpaqueId(req.reviewId, "shk_") ||
        req.expectedRevision != 1)
        return out(Status::invalid);
    const auto review = reviews_.find(req.actorId, req.backendId, req.reviewId);
    if (review.status == MediaShrinkReviewStatus::notFound)
        return out(Status::reviewConflict);
    if (!review.ok() || !fingerprint(review.review.sourceFingerprint))
        return out(Status::reviewConflict);
    const bool fresh = review.review.state == "staged" &&
        review.review.revision == req.expectedRevision;
    const bool claimed = (review.review.state == "publishing" &&
        review.review.revision == req.expectedRevision + 1) ||
        (review.review.state == "published" &&
        review.review.revision == req.expectedRevision + 2);
    if (!fresh && !claimed) return out(Status::reviewConflict);
    if (claimed) {
        // No second publish operation, even after an unknown outcome.
        const auto original = operations_.findByIdempotencyScope(
            req.actorId, req.backendId, "MediaShrinkReview", req.reviewId,
            "media.shrink.publish", req.idempotencyKey);
        if (!original.ok() ||
            original.operation.operationId != review.review.publishOperationId)
            return out(Status::reviewConflict);
    }
    std::uint64_t generation = 0;
    if (!authority_.allowPublish(req.actorId, req.backendId, req.reviewId,
                                 review.review.sourceFingerprint, req.now,
                                 generation) ||
        generation == 0 || generation != review.review.backendGeneration)
        return out(Status::forbidden);
    const auto serialized = publishPayload(req.reviewId, req.expectedRevision);
    Selection selection;
    if (!vdrsuite::mediatools::parseMediaShrinkAgentSelection(
            vdrsuite::mediatools::kMediaShrinkPublishCommandType,
            serialized, selection))
        return out(Status::invalid);
    const std::string requestFingerprint = actionFingerprint(
        "publish", req.backendId, req.reviewId,
        review.review.sourceFingerprint, serialized);
    if (requestFingerprint.size() > 512) return out(Status::invalid);
    const auto proposed = newOperation(
        req.actorId, req.backendId, req.idempotencyKey, generation,
        "MediaShrinkReview", req.reviewId, "media.shrink.publish",
        std::to_string(req.expectedRevision), review.review.sourceFingerprint,
        requestFingerprint, MutationOperationVerificationPolicy::
            reconciliationRequired, req.now);
    if (!mutationOperationIdCanonical(proposed.operationId))
        return out(Status::unavailable);
    auto result = reserve(operations_, proposed,
        vdrsuite::mediatools::kMediaShrinkPublishCommandType, serialized);
    if (result.status != Status::accepted &&
        result.status != Status::replayed) return result;
    if (claimed) {
        // Idempotent observation only; never reassign or re-execute.
        return result.status == Status::replayed &&
               result.operation.operationId == review.review.publishOperationId
            ? result : out(Status::reviewConflict);
    }
    // Reserve first, then exact CAS claim: a failed claim leaves an inert
    // accepted Core operation, never an independently executable command.
    // Crash/replay of an already claimed review returns the same operation.
    const auto claimResult = reviews_.claimPublication(
        req.actorId, req.backendId, req.reviewId, req.expectedRevision,
        result.operation.operationId);
    if (claimResult.status != MediaShrinkReviewStatus::ok &&
        claimResult.status != MediaShrinkReviewStatus::replayed)
        return out(Status::reviewConflict);
    return result;
}
} // namespace vdrsuite::addons
