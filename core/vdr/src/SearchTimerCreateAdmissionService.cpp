#include "SearchTimerCreateAdmissionService.h"

#include "MutationOperationIdentity.h"
#include "MutationOperationRepository.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>

namespace
{
constexpr const char* kResourceType = "SearchTimer";
constexpr const char* kResourceId = "new";
constexpr const char* kActionFamily = "searchtimer.create";
constexpr const char* kPayloadType = "searchtimer.create";
constexpr const char* kExpectedRevision = "new";
// Version 1 lacked backend-owned margins; never dispatch an old
// accepted rule with a silently assumed zero-minute margin.
constexpr std::uint32_t kPayloadVersion = 2U;

bool boundedPrintableAscii(const std::string& value, std::size_t max)
{
    if (value.empty() || value.size() > max) return false;
    for (unsigned char c : value)
        if (c < 0x21U || c > 0x7eU) return false;
    return true;
}

bool backendIdentity(const std::string& value)
{
    return !value.empty() && value.size() <= 128U &&
        value != "." && value != ".." && value != "*" &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '.' || c == '-' || c == '_';
        });
}

bool validRequest(const SearchTimerCreateAdmissionRequest& request)
{
    return boundedPrintableAscii(request.actorId, 160U) &&
        backendIdentity(request.backendId) &&
        boundedPrintableAscii(request.idempotencyKey, 160U) &&
        request.backendGeneration > 0U &&
        request.backendGeneration <= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) &&
        request.requestedAt > 0 &&
        searchTimerCreateOperationPayloadValid(request.payload);
}

SearchTimerCreateAdmissionResult result(
    SearchTimerCreateAdmissionStatus status,
    const vdrsuite::operations::MutationOperation& operation = {})
{
    SearchTimerCreateAdmissionResult item;
    item.status = status;
    item.operation = operation;
    return item;
}
}

SearchTimerCreateAdmissionService::SearchTimerCreateAdmissionService(
    vdrsuite::operations::MutationOperationRepository& repository)
    : repository_(repository)
{
}

SearchTimerCreateAdmissionResult
SearchTimerCreateAdmissionService::replayIfPresent(
    const SearchTimerCreateAdmissionRequest& request,
    const std::string& fingerprint, bool& present)
{
    using namespace vdrsuite::operations;
    present = false;
    const auto found = repository_.findByIdempotencyScope(
        request.actorId, request.backendId,
        kResourceType, kResourceId, kActionFamily, request.idempotencyKey);
    if (found.status == MutationOperationRepositoryStatus::notFound)
        return result(SearchTimerCreateAdmissionStatus::prepared);
    if (found.status != MutationOperationRepositoryStatus::ok)
        return result(SearchTimerCreateAdmissionStatus::repositoryUnavailable);
    present = true;
    const auto& operation = found.operation;
    if (operation.actorId != request.actorId ||
        operation.backendId != request.backendId ||
        operation.idempotencyKey != request.idempotencyKey ||
        operation.resourceType != kResourceType ||
        operation.resourceId != kResourceId ||
        operation.actionFamily != kActionFamily ||
        operation.expectedRevision != kExpectedRevision ||
        operation.verificationPolicy !=
            MutationOperationVerificationPolicy::readbackRequired)
        return result(SearchTimerCreateAdmissionStatus::operationConflict);
    if (operation.requestFingerprint != fingerprint ||
        operation.expectedResourceFingerprint != fingerprint)
        return result(SearchTimerCreateAdmissionStatus::idempotencyConflict,
            operation);

    // A revoked/replaced backend generation never turns into a new write.
    // The durable earlier operation is returned for explicit reconciliation.
    const auto payload = repository_.findPayloadByOperationId(
        operation.operationId);
    if (!payload.ok() || payload.payload.payloadType != kPayloadType ||
        payload.payload.payloadVersion != kPayloadVersion ||
        payload.payload.payloadFingerprint != fingerprint ||
        payload.payload.payload !=
            serializeSearchTimerCreateOperationPayload(request.payload))
        return result(SearchTimerCreateAdmissionStatus::operationConflict,
            operation);

    return result(SearchTimerCreateAdmissionStatus::replayed, operation);
}

SearchTimerCreateAdmissionResult SearchTimerCreateAdmissionService::admit(
    const SearchTimerCreateAdmissionRequest& request)
{
    using namespace vdrsuite::operations;
    if (!validRequest(request))
        return result(SearchTimerCreateAdmissionStatus::invalid);
    const std::string serialized =
        serializeSearchTimerCreateOperationPayload(request.payload);
    const std::string fingerprint =
        searchTimerCreateOperationPayloadFingerprint(request.payload);
    if (serialized.empty() || fingerprint.empty())
        return result(SearchTimerCreateAdmissionStatus::invalid);

    bool present = false;
    auto replay = replayIfPresent(request, fingerprint, present);
    if (present || replay.status != SearchTimerCreateAdmissionStatus::prepared)
        return replay;

    const std::string operationId = generateMutationOperationId();
    if (!mutationOperationIdCanonical(operationId))
        return result(SearchTimerCreateAdmissionStatus::identityUnavailable);

    MutationOperation operation;
    operation.operationId = operationId;
    operation.idempotencyKey = request.idempotencyKey;
    operation.actorId = request.actorId;
    operation.backendId = request.backendId;
    operation.backendGeneration = request.backendGeneration;
    operation.resourceType = kResourceType;
    operation.resourceId = kResourceId;
    operation.expectedRevision = kExpectedRevision;
    operation.expectedResourceFingerprint = fingerprint;
    operation.actionFamily = kActionFamily;
    operation.requestFingerprint = fingerprint;
    operation.requestedAt = request.requestedAt;
    operation.updatedAt = request.requestedAt;
    operation.verificationPolicy =
        MutationOperationVerificationPolicy::readbackRequired;

    MutationOperationPayload payload;
    payload.operationId = operationId;
    payload.payloadType = kPayloadType;
    payload.payloadVersion = kPayloadVersion;
    payload.payload = serialized;
    payload.payloadFingerprint = fingerprint;

    const auto reserved = repository_.reserveWithPayload(operation, payload);
    switch (reserved.status)
    {
        case MutationOperationRepositoryStatus::ok:
            return result(SearchTimerCreateAdmissionStatus::prepared,
                reserved.operation);
        case MutationOperationRepositoryStatus::idempotencyConflict:
        case MutationOperationRepositoryStatus::idempotentReplay:
            // Includes a concurrent first-write race. Do not make another
            // mutation: resolve against the single durable idempotency scope.
            replay = replayIfPresent(request, fingerprint, present);
            return replay.status == SearchTimerCreateAdmissionStatus::prepared
                ? result(SearchTimerCreateAdmissionStatus::repositoryUnavailable)
                : replay;
        case MutationOperationRepositoryStatus::operationConflict:
            return result(SearchTimerCreateAdmissionStatus::operationConflict,
                reserved.operation);
        case MutationOperationRepositoryStatus::invalid:
            return result(SearchTimerCreateAdmissionStatus::invalid);
        default:
            return result(SearchTimerCreateAdmissionStatus::repositoryUnavailable);
    }
}
