#include "MediaImportOperationAdmission.h"

#include "BackendAgentLifecycle.h"
#include "MediaImportOperationFence.h"
#include "MediaToolsAgentExecutor.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"

#include <cstdint>
#include <limits>
#include <string>

namespace vdrsuite::addons {
namespace {
using Status = MediaImportOperationAdmissionStatus;
using Result = MediaImportOperationAdmissionResult;
using namespace vdrsuite::operations;

Result output(Status status, const MutationOperation& operation = {})
{
    Result r;
    r.status = status;
    r.operation = operation;
    return r;
}
bool validIdentity(const std::string& text)
{
    return !text.empty() && text.size() <= 160U &&
        text.find_first_of("/\\\r\n\t") == std::string::npos;
}
std::string serializeSelection(const std::string& id)
{
    return "{\"sourceId\":\"" + id + "\"}";
}
std::string requestFingerprint(const std::string& backend,
                               const std::string& source)
{
    return "media-import-api/1|" + std::to_string(backend.size()) + ":" +
        backend + "|" + std::to_string(source.size()) + ":" + source;
}
bool replayMatches(MutationOperationRepository& repo,
                   const MutationOperation& op,
                   const MediaImportOperationAdmissionRequest& req,
                   const std::string& fingerprint,
                   const std::string& serialized)
{
    if (op.actorId != req.actorId || op.backendId != req.backendId ||
        op.resourceType != "MediaImportSource" ||
        op.resourceId != req.sourceId ||
        op.actionFamily != "media.import" ||
        op.idempotencyKey != req.idempotencyKey ||
        op.requestFingerprint != fingerprint ||
        op.verificationPolicy !=
            MutationOperationVerificationPolicy::reconciliationRequired)
        return false;
    const auto p = repo.findPayloadByOperationId(op.operationId);
    return p.ok() && p.payload.payloadType == "addon.media.import" &&
        p.payload.payloadVersion == 1 &&
        p.payload.payload == serialized &&
        p.payload.payloadFingerprint == fingerprint;
}
} // namespace

MediaImportOperationAdmissionService::MediaImportOperationAdmissionService(
    MutationOperationRepository& operations,
    BackendAgentRepository& agents,
    MediaImportOperationFence& fence)
    : operations_(operations), agents_(agents), fence_(fence)
{
}

MediaImportOperationAdmissionResult
MediaImportOperationAdmissionService::admit(
    const MediaImportOperationAdmissionRequest& req)
{
    const std::string serialized = serializeSelection(req.sourceId);
    std::string checkedSource;
    if (req.now <= 0 || req.now > std::numeric_limits<std::int64_t>::max()-900 ||
        !validIdentity(req.actorId) || !validIdentity(req.backendId) ||
        req.backendId == "*" || !validIdentity(req.idempotencyKey) ||
        req.sourceId.size() < 16U || req.sourceId.size() > 64U ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            serialized, checkedSource) ||
        checkedSource != req.sourceId)
        return output(Status::invalid);

    // Core rechecks agent lease/capability, active actor, effective exact
    // backend grant, writable backend and audited administrator enable.
    std::string reason;
    if (!fence_.effectiveMediaImportCapability(
            req.actorId, req.backendId, req.now, reason))
        return output(Status::forbidden);

    const auto agent = agents_.findAgentForBackend(req.backendId);
    if (!agent || agent->revoked || agent->incompatible ||
        agent->backendGeneration == 0 ||
        agent->backendGeneration >
            static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()) ||
        agent->leaseExpiresAt < req.now)
        return output(Status::forbidden);

    const std::string fingerprint =
        requestFingerprint(req.backendId, req.sourceId);
    if (fingerprint.size() > 512U) return output(Status::invalid);

    const auto previous = operations_.findByIdempotencyScope(
        req.actorId, req.backendId, "MediaImportSource", req.sourceId,
        "media.import", req.idempotencyKey);
    if (previous.status == MutationOperationRepositoryStatus::ok)
    {
        if (!replayMatches(operations_, previous.operation,
                           req, fingerprint, serialized) ||
            previous.operation.backendGeneration != agent->backendGeneration)
            return output(Status::idempotencyConflict);
        return output(Status::replayed, previous.operation);
    }
    if (previous.status != MutationOperationRepositoryStatus::notFound)
        return output(Status::unavailable);

    MutationOperation op;
    op.operationId = generateMutationOperationId();
    if (!mutationOperationIdCanonical(op.operationId))
        return output(Status::unavailable);
    op.idempotencyKey = req.idempotencyKey;
    op.actorId = req.actorId;
    op.backendId = req.backendId;
    op.backendGeneration = agent->backendGeneration;
    op.resourceType = "MediaImportSource";
    op.resourceId = req.sourceId;
    op.expectedRevision = "selection-v1";
    op.actionFamily = "media.import";
    op.requestFingerprint = fingerprint;
    op.requestedAt = req.now;
    op.deadline = req.now + 900;
    op.verificationPolicy =
        MutationOperationVerificationPolicy::reconciliationRequired;
    op.state = MutationOperationState::accepted;
    op.updatedAt = req.now;

    MutationOperationPayload payload;
    payload.operationId = op.operationId;
    payload.payloadType = "addon.media.import";
    payload.payloadVersion = 1;
    payload.payload = serialized;
    payload.payloadFingerprint = fingerprint;

    const auto reserved = operations_.reserveWithPayload(op, payload);
    if (reserved.status == MutationOperationRepositoryStatus::ok)
        return output(Status::accepted, reserved.operation);
    if (reserved.status == MutationOperationRepositoryStatus::idempotencyConflict)
    {
        // A concurrent same-key reservation may win. Never return a
        // second operation ID or create a second Agent job.
        const auto raced = operations_.findByIdempotencyScope(
            req.actorId, req.backendId, "MediaImportSource", req.sourceId,
            "media.import", req.idempotencyKey);
        if (raced.ok() && replayMatches(
                operations_, raced.operation, req, fingerprint, serialized) &&
            raced.operation.backendGeneration == agent->backendGeneration)
            return output(Status::replayed, raced.operation);
        return output(Status::idempotencyConflict);
    }
    if (reserved.status == MutationOperationRepositoryStatus::operationConflict)
        return output(Status::conflict);
    if (reserved.status == MutationOperationRepositoryStatus::invalid)
        return output(Status::invalid);
    return output(Status::unavailable);
}

} // namespace vdrsuite::addons
