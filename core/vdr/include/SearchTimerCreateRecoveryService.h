#pragma once

#include "MutationOperation.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations { class MutationOperationRepository; }

enum class SearchTimerCreateRecoveryStatus
{
    markedUnknown,
    alreadyUnknown,
    notExpired,
    denied,
    conflict,
    unavailable,
};

struct SearchTimerCreateRecoveryRequest
{
    std::string operationId;
    std::string actorId;
    std::string backendId;
    std::uint64_t backendGeneration = 0U;
    std::int64_t now = 0;
    std::int64_t staleAfterSeconds = 0;
    // Set only by a supervisor that has independently established that
    // the prior dispatcher no longer owns this operation.
    bool dispatcherStopped = false;
};

struct SearchTimerCreateRecoveryResult
{
    SearchTimerCreateRecoveryStatus status =
        SearchTimerCreateRecoveryStatus::unavailable;
    vdrsuite::operations::MutationOperation operation;
};

// Dormant domain-only crash recovery, not an HTTP handler or a daemon poller.
// Never calls RESTfulAPI and never transitions to "accepted" or "succeeded".
// A fresh owner may mark an orphaned in-flight CREATE as outcome_unknown,
// preserving any durable evidence for separate, authoritative reconciliation.
class SearchTimerCreateRecoveryService
{
public:
    explicit SearchTimerCreateRecoveryService(
        vdrsuite::operations::MutationOperationRepository& operations);

    SearchTimerCreateRecoveryResult markOrphanedUnknown(
        const SearchTimerCreateRecoveryRequest& request);
private:
    vdrsuite::operations::MutationOperationRepository& operations_;
};
