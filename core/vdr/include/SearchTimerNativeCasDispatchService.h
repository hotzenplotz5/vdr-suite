#pragma once

#include "MutationOperation.h"
#include "SearchTimerNativeCasContract.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations { class MutationOperationRepository; }
class ISearchTimerBackendGenerationAuthority;

// Internal, OFFLINE-ONLY until epgsearch has an audited native CAS writer.
// This service does not expose routes or perform readback/completion.
enum class SearchTimerNativeCasDispatchStatus
{
    dispatchedUnverified,
    failedVerified,
    outcomeUnknown,
    denied,
    conflict,
    unavailable,
    unsupported,
};

struct SearchTimerNativeCasDispatchRequest
{
    std::string operationId;
    std::string actorId;
    std::uint64_t backendGeneration = 0;
    std::int64_t now = 0;
    bool backendWriteAuthorized = false;
    bool operatorConfirmed = false;
    SearchTimerNativeCasRequest native;
};

struct SearchTimerNativeCasDispatchResult
{
    SearchTimerNativeCasDispatchStatus status =
        SearchTimerNativeCasDispatchStatus::unavailable;
    SearchTimerNativeCasStatus nativeStatus =
        SearchTimerNativeCasStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Canonical immutable admission evidence, NOT a client-controlled flag.
// The future admission path must atomically reserve these exact bytes and
// their SHA-256 fingerprint in MutationOperationRepository.
std::string serializeSearchTimerNativeCasEvidence(
    const SearchTimerNativeCasRequest& request);
std::string searchTimerNativeCasEvidenceFingerprint(
    const SearchTimerNativeCasRequest& request);

class SearchTimerNativeCasDispatchService final
{
public:
    explicit SearchTimerNativeCasDispatchService(
        vdrsuite::operations::MutationOperationRepository& operations,
        const ISearchTimerBackendGenerationAuthority& generations);

    SearchTimerNativeCasDispatchResult dispatchOnce(
        const SearchTimerNativeCasDispatchRequest& request,
        ISearchTimerNativeCasProvider& provider);
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
    const ISearchTimerBackendGenerationAuthority& generations_;
};
