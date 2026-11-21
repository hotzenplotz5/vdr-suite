#pragma once

#include "MutationOperation.h"
#include "SearchTimerCreateOperationPayload.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations { class MutationOperationRepository; }

enum class SearchTimerCreateAdmissionStatus
{
    prepared,
    replayed,
    invalid,
    idempotencyConflict,
    operationConflict,
    identityUnavailable,
    repositoryUnavailable,
};

struct SearchTimerCreateAdmissionRequest
{
    std::string actorId;
    std::string backendId;
    std::uint64_t backendGeneration = 0U;
    std::string idempotencyKey;
    std::int64_t requestedAt = 0;
    SearchTimerCreateOperationPayload payload;
};

struct SearchTimerCreateAdmissionResult
{
    SearchTimerCreateAdmissionStatus status =
        SearchTimerCreateAdmissionStatus::repositoryUnavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Persistence-only gate. No network calls and no provider mutations.
// A separate, explicitly authorized dispatcher must be added and proven
// before Public-v1 can expose an actual SearchTimer CREATE operation.
class SearchTimerCreateAdmissionService
{
public:
    explicit SearchTimerCreateAdmissionService(
        vdrsuite::operations::MutationOperationRepository& repository);

    SearchTimerCreateAdmissionResult admit(
        const SearchTimerCreateAdmissionRequest& request);

private:
    SearchTimerCreateAdmissionResult replayIfPresent(
        const SearchTimerCreateAdmissionRequest& request,
        const std::string& fingerprint, bool& present);

    vdrsuite::operations::MutationOperationRepository& repository_;
};
