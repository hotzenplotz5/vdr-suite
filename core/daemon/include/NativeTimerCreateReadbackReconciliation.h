#pragma once

#include "NativeTimerCreateOperationCompletionService.h"
#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackExpectation.h"
#include "NativeTimerCreateReadbackVerificationService.h"
#include "TimerAssignmentFulfillmentService.h"

#include <cstdint>
#include <string>

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::daemon
{

enum class NativeTimerCreateReadbackReconciliationStatus
{
    completed,
    alreadyCompleted,
    payloadNotFound,
    payloadInvalid,
    identityConflict,
    staleReconciliationTime,
    readbackVerificationFailed,
    fulfillmentFailed,
    completionFailed,
};

struct NativeTimerCreateReadbackReconciliationResult
{
    NativeTimerCreateReadbackReconciliationStatus status =
        NativeTimerCreateReadbackReconciliationStatus::completionFailed;
    vdrsuite::timers::NativeTimerCreateReadbackVerificationResult verification;
    vdrsuite::timers::TimerAssignmentFulfillmentResult fulfillment;
    vdrsuite::timers::NativeTimerCreateOperationCompletionResult completion;
    std::string reasonCode;

    bool ok() const
    {
        return status == NativeTimerCreateReadbackReconciliationStatus::completed ||
            status ==
                NativeTimerCreateReadbackReconciliationStatus::alreadyCompleted;
    }
};

// Stateless daemon-composition adapter for the already accepted post-dispatch
// lifecycle. It consumes one existing CREATE readback expectation plus one
// authoritative complete readback evidence object and delegates every durable
// transition to the existing repository/service owners. It owns no persistence,
// performs no native read and cannot make an Agent command pollable.
NativeTimerCreateReadbackReconciliationResult
reconcileNativeTimerCreateReadback(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerCreateReadbackVerificationService&
        verificationService,
    vdrsuite::timers::TimerAssignmentFulfillmentService& fulfillmentService,
    vdrsuite::timers::NativeTimerCreateOperationCompletionService&
        completionService,
    const vdrsuite::timers::NativeTimerCreateReadbackExpectation& expectation,
    const vdrsuite::timers::NativeTimerCreateReadbackEvidence& readbackEvidence,
    std::int64_t reconciledAt);

} // namespace vdrsuite::daemon
