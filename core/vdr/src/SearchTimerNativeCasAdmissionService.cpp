#include "SearchTimerNativeCasAdmissionService.h"

#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"
#include "SearchTimerNativeCasDispatchService.h"

#include <cstdint>
#include <limits>
#include <string>

namespace
{
using namespace vdrsuite::operations;

bool safeAsciiIdentity(const std::string& text)
{
    if (text.empty() || text.size() > 160U) return false;
    for (unsigned char c : text)
        if (c < 0x21U || c > 0x7eU) return false;
    return true;
}

const char* family(SearchTimerNativeCasAction action)
{
    switch (action)
    {
        case SearchTimerNativeCasAction::update:
            return "searchtimer.native-cas.update";
        case SearchTimerNativeCasAction::activate:
            return "searchtimer.native-cas.activate";
        case SearchTimerNativeCasAction::deactivate:
            return "searchtimer.native-cas.deactivate";
        case SearchTimerNativeCasAction::remove:
            return "searchtimer.native-cas.delete";
    }
    return nullptr;
}

bool valid(const SearchTimerNativeCasAdmissionRequest& req)
{
    return safeAsciiIdentity(req.actorId) &&
        safeAsciiIdentity(req.idempotencyKey) &&
        req.backendGeneration > 0U &&
        req.backendGeneration <= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) &&
        req.requestedAt > 0 &&
        SearchTimerNativeCasContract::validNativeEvidence(req.native);
}

SearchTimerNativeCasAdmissionResult result(
    SearchTimerNativeCasAdmissionStatus status,
    const MutationOperation& op = {})
{
    SearchTimerNativeCasAdmissionResult value;
    value.status = status;
    value.operation = op;
    return value;
}
}

SearchTimerNativeCasAdmissionService::SearchTimerNativeCasAdmissionService(
    MutationOperationRepository& repository) : repository_(repository)
{
}

SearchTimerNativeCasAdmissionResult
SearchTimerNativeCasAdmissionService::replayIfPresent(
    const SearchTimerNativeCasAdmissionRequest& req,
    const std::string& fingerprint,
    const std::string& evidence, bool& present)
{
    present = false;
    const auto found = repository_.findByIdempotencyScope(
        req.actorId, req.native.backendId, "SearchTimer",
        req.native.publicSearchTimerId, family(req.native.action),
        req.idempotencyKey);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return result(SearchTimerNativeCasAdmissionStatus::prepared);
    if (found.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerNativeCasAdmissionStatus::repositoryUnavailable);
    present = true;
    const auto& op = found.operation;
    if (op.actorId != req.actorId ||
        op.backendId != req.native.backendId ||
        op.idempotencyKey != req.idempotencyKey ||
        op.resourceType != "SearchTimer" ||
        op.resourceId != req.native.publicSearchTimerId ||
        op.actionFamily != family(req.native.action) ||
        op.expectedRevision != req.native.expectedRevision ||
        op.expectedResourceFingerprint != req.native.expectedGeneration ||
        op.verificationPolicy !=
            MutationOperationVerificationPolicy::readbackRequired)
        return result(SearchTimerNativeCasAdmissionStatus::operationConflict, op);

    if (op.requestFingerprint != fingerprint)
        return result(SearchTimerNativeCasAdmissionStatus::idempotencyConflict,
            op);
    const auto stored = repository_.findPayloadByOperationId(op.operationId);
    if (!stored.ok() ||
        stored.payload.payloadType != "searchtimer.native-cas" ||
        stored.payload.payloadVersion != 1U ||
        stored.payload.payload != evidence ||
        stored.payload.payloadFingerprint != fingerprint)
        return result(SearchTimerNativeCasAdmissionStatus::operationConflict, op);

    // Return original operation even when the observed backend generation
    // has since advanced. A replay is NOT permission to re-dispatch.
    return result(SearchTimerNativeCasAdmissionStatus::replayed, op);
}

SearchTimerNativeCasAdmissionResult SearchTimerNativeCasAdmissionService::admit(
    const SearchTimerNativeCasAdmissionRequest& req)
{
    if (!valid(req))
        return result(SearchTimerNativeCasAdmissionStatus::invalid);
    const std::string evidence =
        serializeSearchTimerNativeCasEvidence(req.native);
    const std::string fingerprint =
        searchTimerNativeCasEvidenceFingerprint(req.native);
    if (evidence.empty() || fingerprint.empty())
        return result(SearchTimerNativeCasAdmissionStatus::invalid);

    bool present = false;
    auto replay = replayIfPresent(req, fingerprint, evidence, present);
    if (present ||
        replay.status != SearchTimerNativeCasAdmissionStatus::prepared)
        return replay;

    const std::string id = generateMutationOperationId();
    if (!mutationOperationIdCanonical(id))
        return result(SearchTimerNativeCasAdmissionStatus::identityUnavailable);

    MutationOperation op;
    op.operationId = id;
    op.idempotencyKey = req.idempotencyKey;
    op.actorId = req.actorId;
    op.backendId = req.native.backendId;
    op.backendGeneration = req.backendGeneration;
    op.resourceType = "SearchTimer";
    op.resourceId = req.native.publicSearchTimerId;
    op.expectedRevision = req.native.expectedRevision;
    op.expectedResourceFingerprint = req.native.expectedGeneration;
    op.actionFamily = family(req.native.action);
    op.requestFingerprint = fingerprint;
    op.requestedAt = req.requestedAt;
    op.updatedAt = req.requestedAt;
    op.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;

    MutationOperationPayload stored;
    stored.operationId = id;
    stored.payloadType = "searchtimer.native-cas";
    stored.payloadVersion = 1U;
    stored.payload = evidence;
    stored.payloadFingerprint = fingerprint;
    const auto reserved = repository_.reserveWithPayload(op, stored);
    switch (reserved.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(SearchTimerNativeCasAdmissionStatus::prepared,
                reserved.operation);
        case MutationOperationRepositoryStatus::idempotentReplay:
        case MutationOperationRepositoryStatus::idempotencyConflict:
            replay = replayIfPresent(req, fingerprint, evidence, present);
            return replay.status == SearchTimerNativeCasAdmissionStatus::prepared
                ? result(SearchTimerNativeCasAdmissionStatus::repositoryUnavailable)
                : replay;
        case MutationOperationRepositoryStatus::operationConflict:
            return result(SearchTimerNativeCasAdmissionStatus::operationConflict,
                reserved.operation);
        case MutationOperationRepositoryStatus::invalid:
            return result(SearchTimerNativeCasAdmissionStatus::invalid);
        default:
            return result(SearchTimerNativeCasAdmissionStatus::repositoryUnavailable);
    }
}
