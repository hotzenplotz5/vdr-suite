#include "SearchTimerNativeCasDispatchService.h"

#include "MutationOperationRepository.h"
#include "SearchTimerCreateDeviceAdmissionService.h"

#include <openssl/sha.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>

namespace
{
using namespace vdrsuite::operations;

constexpr const char* kPayloadType = "searchtimer.native-cas";
constexpr std::uint32_t kPayloadVersion = 1U;

const char* actionName(SearchTimerNativeCasAction action)
{
    switch (action)
    {
        case SearchTimerNativeCasAction::update: return "update";
        case SearchTimerNativeCasAction::activate: return "activate";
        case SearchTimerNativeCasAction::deactivate: return "deactivate";
        case SearchTimerNativeCasAction::remove: return "delete";
    }
    return nullptr;
}

void addField(std::string& data, const std::string& value)
{
    data += std::to_string(value.size());
    data += ':';
    data += value;
    data += '|';
}

SearchTimerNativeCasDispatchResult result(
    SearchTimerNativeCasDispatchStatus status,
    const MutationOperation& operation = {},
    SearchTimerNativeCasStatus nativeStatus =
        SearchTimerNativeCasStatus::unavailable)
{
    SearchTimerNativeCasDispatchResult out;
    out.status = status;
    out.operation = operation;
    out.nativeStatus = nativeStatus;
    return out;
}

bool exactOperation(const MutationOperation& operation,
    const SearchTimerNativeCasDispatchRequest& req,
    const std::string& fingerprint)
{
    const char* action = actionName(req.native.action);
    return action != nullptr &&
        operation.operationId == req.operationId &&
        operation.actorId == req.actorId &&
        operation.backendId == req.native.backendId &&
        operation.backendGeneration == req.backendGeneration &&
        operation.resourceType == "SearchTimer" &&
        operation.resourceId == req.native.publicSearchTimerId &&
        operation.expectedRevision == req.native.expectedRevision &&
        operation.expectedResourceFingerprint ==
            req.native.expectedGeneration &&
        operation.actionFamily ==
            std::string("searchtimer.native-cas.") + action &&
        operation.requestFingerprint == fingerprint &&
        operation.verificationPolicy ==
            MutationOperationVerificationPolicy::readbackRequired;
}
} // namespace

std::string serializeSearchTimerNativeCasEvidence(
    const SearchTimerNativeCasRequest& request)
{
    const char* action = actionName(request.action);
    if (!action || request.backendId.empty() ||
        request.publicSearchTimerId.empty() ||
        request.nativeId.empty() ||
        request.expectedGeneration.empty() ||
        request.expectedRevision.empty() ||
        request.changePayload.size() > 8192U)
        return {};
    std::string data = "searchtimer-native-cas-evidence/1|";
    addField(data, action);
    addField(data, request.backendId);
    addField(data, request.publicSearchTimerId);
    addField(data, request.nativeId);
    addField(data, request.expectedGeneration);
    addField(data, request.expectedRevision);
    addField(data, request.changePayload);
    return data;
}

std::string searchTimerNativeCasEvidenceFingerprint(
    const SearchTimerNativeCasRequest& request)
{
    const std::string bytes = serializeSearchTimerNativeCasEvidence(request);
    if (bytes.empty()) return {};
    std::array<unsigned char, SHA256_DIGEST_LENGTH> digest{};
    if (!SHA256(reinterpret_cast<const unsigned char*>(bytes.data()),
                bytes.size(), digest.data())) return {};
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result = "sha256:";
    for (const auto byte : digest)
    {
        result += kHex[byte >> 4U];
        result += kHex[byte & 15U];
    }
    return result;
}

SearchTimerNativeCasDispatchService::SearchTimerNativeCasDispatchService(
    MutationOperationRepository& operations,
    const ISearchTimerBackendGenerationAuthority& generations)
    : operations_(operations), generations_(generations)
{
}

SearchTimerNativeCasDispatchResult
SearchTimerNativeCasDispatchService::dispatchOnce(
    const SearchTimerNativeCasDispatchRequest& req,
    ISearchTimerNativeCasProvider& provider)
{
    if (req.operationId.empty() || req.actorId.empty() ||
        req.backendGeneration == 0U || req.now <= 0 ||
        !req.backendWriteAuthorized || !req.operatorConfirmed)
        return result(SearchTimerNativeCasDispatchStatus::denied);

    const std::string serialized =
        serializeSearchTimerNativeCasEvidence(req.native);
    const std::string fingerprint =
        searchTimerNativeCasEvidenceFingerprint(req.native);
    if (serialized.empty() || fingerprint.empty())
        return result(SearchTimerNativeCasDispatchStatus::denied);

    const auto found = operations_.findById(req.operationId);
    if (!found.ok())
        return result(SearchTimerNativeCasDispatchStatus::unavailable);
    const auto& operation = found.operation;
    if (!exactOperation(operation, req, fingerprint))
        return result(SearchTimerNativeCasDispatchStatus::denied, operation);
    if (operation.state != MutationOperationState::accepted)
        return result(SearchTimerNativeCasDispatchStatus::conflict, operation);
    if (req.now < operation.updatedAt)
        return result(SearchTimerNativeCasDispatchStatus::denied, operation);

    const auto payload =
        operations_.findPayloadByOperationId(operation.operationId);
    if (!payload.ok() ||
        payload.payload.payloadType != kPayloadType ||
        payload.payload.payloadVersion != kPayloadVersion ||
        payload.payload.payload == "" ||
        payload.payload.payload != serialized ||
        payload.payload.payloadFingerprint != fingerprint)
        return result(SearchTimerNativeCasDispatchStatus::unavailable,
            operation);

    // The durable generation alone is not evidence that this is still the
    // same backend instance. Revalidate against the server-owned authority
    // before claiming a one-shot operation. Native CAS must still protect
    // against races that occur after this read.
    std::optional<std::uint64_t> liveGeneration;
    try { liveGeneration = generations_.currentGeneration(operation.backendId); }
    catch (...) { return result(SearchTimerNativeCasDispatchStatus::unavailable,
        operation); }
    if (!liveGeneration.has_value() ||
        *liveGeneration != operation.backendGeneration)
        return result(SearchTimerNativeCasDispatchStatus::unavailable,
            operation);

    // A missing native versioned capability never consumes an operation.
    // This is NOT a proof that a future capability will remain available.
    try
    {
        if (!provider.supportsAtomicGenerationCasV1())
            return result(SearchTimerNativeCasDispatchStatus::unsupported,
                operation);
    }
    catch (...)
    {
        return result(SearchTimerNativeCasDispatchStatus::unavailable,
            operation);
    }

    const auto claimed = operations_.transition(
        operation.operationId, operation.operationRevision,
        MutationOperationState::accepted, MutationOperationState::dispatching,
        "", req.now);
    if (!claimed.ok())
        return result(SearchTimerNativeCasDispatchStatus::conflict,
            claimed.operation);

    // Derive the claim and backend permission here, never from flags passed
    // in req.native. The durable payload is immutable and exactly matched.
    auto command = req.native;
    command.operationId = operation.operationId;
    command.requestFingerprint = fingerprint;
    command.durableOperationClaimed = true;
    command.backendWriteAuthorized = true;
    command.operatorConfirmed = req.operatorConfirmed;

    const SearchTimerNativeCasStatus native =
        SearchTimerNativeCasContract::invokeOnce(command, provider);
    MutationOperationState next = MutationOperationState::outcomeUnknown;
    SearchTimerNativeCasDispatchStatus status =
        SearchTimerNativeCasDispatchStatus::outcomeUnknown;

    if (native == SearchTimerNativeCasStatus::applied)
    {
        // A native acknowledgement is not full Public-v1 success: exact
        // generation readback must independently finish this operation.
        next = MutationOperationState::executedUnverified;
        status = SearchTimerNativeCasDispatchStatus::dispatchedUnverified;
    }
    else if (native == SearchTimerNativeCasStatus::staleGeneration ||
             native == SearchTimerNativeCasStatus::staleRevision ||
             native == SearchTimerNativeCasStatus::notFound)
    {
        // The native reply has explicitly attested zero writes.
        next = MutationOperationState::failedVerified;
        status = SearchTimerNativeCasDispatchStatus::failedVerified;
    }

    const auto finished = operations_.transition(
        claimed.operation.operationId, claimed.operation.operationRevision,
        MutationOperationState::dispatching, next, "", req.now);
    if (!finished.ok())
        return result(SearchTimerNativeCasDispatchStatus::outcomeUnknown,
            claimed.operation, native);
    return result(status, finished.operation, native);
}
