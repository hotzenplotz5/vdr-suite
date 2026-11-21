#include "MediaImportOperationAdmission.h"

#include "BackendAgentLifecycle.h"
#include "Database.h"
#include "MediaImportOperationFence.h"
#include "MediaToolsAgentExecutor.h"
#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"

#include <cstdint>
#include <limits>
#include <sstream>
#include <iomanip>
#include <string>
#include <unordered_set>
#include <vector>

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
std::string serializeSelection(
    const std::string& id, vdrsuite::mediatools::MediaImportDecision decision,
    const std::string& metadata)
{
    return vdrsuite::mediatools::mediaImportSelectionJson(id, decision, metadata);
}
std::string requestFingerprint(const std::string& backend,
                               const std::string& source, vdrsuite::mediatools::MediaImportDecision decision,
                               const std::string& metadata)
{
    // Existing fingerprint remains byte-identical without TMDb. For the
    // one-shot snapshot, bind its bytes to the same idempotency scope without
    // placing the (potentially large) synopsis in a 512-byte fingerprint.
    std::uint64_t digest = 14695981039346656037ULL;
    for (unsigned char ch : metadata) {
        digest ^= ch;
        digest *= 1099511628211ULL;
    }
    std::ostringstream suffix;
    if (!metadata.empty())
        suffix << "|tmdb-snapshot-v1:" << std::hex << std::setfill('0')
               << std::setw(16) << digest;

    return "media-import-api/1|" + std::to_string(backend.size()) + ":" +
        backend + "|" + std::to_string(source.size()) + ":" + source +
        (decision == vdrsuite::mediatools::MediaImportDecision::import
            ? ""
            : decision == vdrsuite::mediatools::MediaImportDecision::local
                ? "|videoDecision:local" // existing v2 fingerprints must replay
                : decision == vdrsuite::mediatools::MediaImportDecision::desktop
                    ? "|videoDecision:desktop"
                     : decision == vdrsuite::mediatools::MediaImportDecision::skip
                         ? "|videoDecision:skip"
                         : decision == vdrsuite::mediatools::MediaImportDecision::duplicate
                             ? "|videoDecision:duplicate"
                             : decision == vdrsuite::mediatools::MediaImportDecision::deleteDuplicate
                                 ? "|videoDecision:delete_duplicate"
                                 : "|videoDecision:release") + suffix.str();
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
        p.payload.payloadVersion ==
            vdrsuite::mediatools::mediaImportSelectionVersion(
                req.videoDecision == "desktop"
                    ? vdrsuite::mediatools::MediaImportDecision::desktop
                    : req.videoDecision == "skip"
                        ? vdrsuite::mediatools::MediaImportDecision::skip
                        : req.videoDecision == "release"
                            ? vdrsuite::mediatools::MediaImportDecision::release
                            : req.videoDecision == "duplicate"
                                ? vdrsuite::mediatools::MediaImportDecision::duplicate
                                : req.videoDecision == "delete_duplicate"
                                    ? vdrsuite::mediatools::MediaImportDecision::deleteDuplicate
                                : req.approveVideoReencode
                            ? vdrsuite::mediatools::MediaImportDecision::local
                            : vdrsuite::mediatools::MediaImportDecision::import,
                serialized) &&
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
    return admitInternal(req, false);
}

MediaImportOperationAdmissionResult
MediaImportOperationAdmissionService::admitInternal(
    const MediaImportOperationAdmissionRequest& req,
    bool inExistingTransaction)
{
    using Decision = vdrsuite::mediatools::MediaImportDecision;
    if (req.approveVideoReencode && !req.videoDecision.empty())
        return output(Status::invalid);
    if (!req.videoDecision.empty() && req.videoDecision != "desktop" &&
        req.videoDecision != "skip" && req.videoDecision != "release" &&
        req.videoDecision != "duplicate" &&
        req.videoDecision != "delete_duplicate")
        return output(Status::invalid);
    const Decision choice = req.videoDecision == "desktop"
        ? Decision::desktop : req.videoDecision == "skip"
            ? Decision::skip : req.videoDecision == "release"
                ? Decision::release : req.videoDecision == "duplicate"
                    ? Decision::duplicate : req.videoDecision == "delete_duplicate"
                    ? Decision::deleteDuplicate : req.approveVideoReencode
                    ? Decision::local : Decision::import;
    const std::string serialized = serializeSelection(
        req.sourceId, choice, req.metadata);
    std::string checkedSource;
    Decision checkedDecision = Decision::import;
    if (req.now <= 0 || req.now > std::numeric_limits<std::int64_t>::max() -
             24 * 3600 ||
        !validIdentity(req.actorId) || !validIdentity(req.backendId) ||
        req.backendId == "*" || !validIdentity(req.idempotencyKey) ||
        req.sourceId.size() < 16U || req.sourceId.size() > 64U ||
        !vdrsuite::mediatools::parseMediaImportSelection(
            serialized, checkedSource, checkedDecision) ||
        checkedSource != req.sourceId || checkedDecision != choice)
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
        requestFingerprint(
            req.backendId, req.sourceId, choice, req.metadata);
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
    op.expectedRevision =
        "selection-v" + std::to_string(
            vdrsuite::mediatools::mediaImportSelectionVersion(choice, serialized));
    op.actionFamily = "media.import";
    op.requestFingerprint = fingerprint;
    op.requestedAt = req.now;
    // One Core-owned deadline for every explicitly approved media action.
    // No independent short Agent or CLI deadline may override this Operation.
    op.deadline = req.now + 24 * 3600;
    op.verificationPolicy =
        MutationOperationVerificationPolicy::reconciliationRequired;
    op.state = MutationOperationState::accepted;
    op.updatedAt = req.now;

    MutationOperationPayload payload;
    payload.operationId = op.operationId;
    payload.payloadType = "addon.media.import";
    payload.payloadVersion =
        vdrsuite::mediatools::mediaImportSelectionVersion(
            choice, serialized);
    payload.payload = serialized;
    payload.payloadFingerprint = fingerprint;

    const auto reserved = inExistingTransaction
        ? operations_.reserveWithPayloadInCurrentTransaction(op, payload)
        : operations_.reserveWithPayload(op, payload);
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

// Reserve the WHOLE confirmed browser selection before any Agent side effect.
// Existing per-source validation, grants, immutable payload and idempotency
// apply to every item; a rejected item rolls the transaction back atomically.
// A browser closing after the HTTP request cannot strand unreserved sources.
MediaImportBatchAdmissionResult
MediaImportOperationAdmissionService::admitBatch(
    const MediaImportBatchAdmissionRequest& req)
{
    MediaImportBatchAdmissionResult out;
    if (req.batchId.size() != 36U ||
        req.batchId.compare(0U, 4U, "mb1_") != 0 ||
        req.sources.size() < 2U || req.sources.size() > 100U)
    {
        out.status = Status::invalid;
        return out;
    }
    for (std::size_t i = 4U; i < req.batchId.size(); ++i)
        if (!((req.batchId[i] >= '0' && req.batchId[i] <= '9') ||
              (req.batchId[i] >= 'a' && req.batchId[i] <= 'f'))) {
            out.status = Status::invalid;
            return out;
        }
    std::unordered_set<std::string> distinct;
    for (const auto& item : req.sources) {
        if (!distinct.insert(item.sourceId).second) {
            out.status = Status::invalid;
            return out;
        }
    }

    auto* db = const_cast<Database*>(operations_.databaseIdentity());
    if (!db) return out;
    auto lease = db->acquireTransactionLease();
    if (!db->execute("BEGIN IMMEDIATE TRANSACTION;")) return out;
    bool inserted = false;
    bool replayed = false;
    for (std::size_t index = 0; index < req.sources.size(); ++index)
    {
        MediaImportOperationAdmissionRequest item;
        item.actorId = req.actorId;
        item.backendId = req.backendId;
        item.sourceId = req.sources[index].sourceId;
        item.metadata = req.sources[index].metadata;
        item.now = req.now;
        std::ostringstream key;
        key << req.batchId << "_" << std::setfill('0') <<
            std::setw(3) << index;
        item.idempotencyKey = key.str();
        // Batch imports only accept a direct, prechecked media source; no
        // implicit re-encode, source deletion, or marking is authorized.
        const auto admitted = admitInternal(item, true);
        if (admitted.status != Status::accepted &&
            admitted.status != Status::replayed)
        {
            db->execute("ROLLBACK;");
            out.operations.clear();
            out.status = admitted.status;
            return out;
        }
        inserted |= admitted.status == Status::accepted;
        replayed |= admitted.status == Status::replayed;
        out.operations.push_back(admitted.operation);
    }
    // A prior successful attempt can only replay entirely. A mixed state
    // could indicate an inconsistent batch identity; never dispatch it.
    if ((inserted && replayed) || !db->execute("COMMIT;"))
    {
        db->execute("ROLLBACK;");
        out.operations.clear();
        out.status = inserted && replayed
            ? Status::idempotencyConflict : Status::unavailable;
        return out;
    }
    out.status = inserted ? Status::accepted : Status::replayed;
    return out;
}

} // namespace vdrsuite::addons
