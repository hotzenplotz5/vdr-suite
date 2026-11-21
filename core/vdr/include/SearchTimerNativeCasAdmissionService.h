#pragma once

#include "MutationOperation.h"
#include "SearchTimerNativeCasContract.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations { class MutationOperationRepository; }

enum class SearchTimerNativeCasAdmissionStatus
{
    prepared,
    replayed,
    invalid,
    idempotencyConflict,
    operationConflict,
    identityUnavailable,
    repositoryUnavailable,
};

struct SearchTimerNativeCasAdmissionRequest
{
    std::string actorId;
    std::uint64_t backendGeneration = 0U;
    std::string idempotencyKey;
    std::int64_t requestedAt = 0;
    // Must originate from an authoritative generation/revision observation.
    // No user-facing route or grant check is implemented here.
    SearchTimerNativeCasRequest native;
};

struct SearchTimerNativeCasAdmissionResult
{
    SearchTimerNativeCasAdmissionStatus status =
        SearchTimerNativeCasAdmissionStatus::repositoryUnavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Persistence only: no provider, no native write, no Public-v1 endpoint.
// Device permission, active-rule preview and native proof are future gates.
class SearchTimerNativeCasAdmissionService final
{
public:
    explicit SearchTimerNativeCasAdmissionService(
        vdrsuite::operations::MutationOperationRepository& repository);

    SearchTimerNativeCasAdmissionResult admit(
        const SearchTimerNativeCasAdmissionRequest& request);
private:
    SearchTimerNativeCasAdmissionResult replayIfPresent(
        const SearchTimerNativeCasAdmissionRequest& request,
        const std::string& fingerprint,
        const std::string& evidence, bool& present);

    vdrsuite::operations::MutationOperationRepository& repository_;
};
