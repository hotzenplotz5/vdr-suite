#pragma once

#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerCreateReadbackExpectation.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

class BackendAgentCommandRepository;

namespace vdrsuite::agent
{
class BackendAgentNativeTimerCreateActivationService;
class BackendAgentNativeTimerCreateReservationService;
}

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::timers
{
class NativeTimerCreateDispatchService;
class NativeTimerCreateOperationCompletionService;
class NativeTimerCreateReadbackVerificationService;
class TimerAssignmentFulfillmentService;
}

namespace vdrsuite::daemon
{

using NativeTimerCreateReadbackAcquirer = std::function<
    std::optional<vdrsuite::timers::NativeTimerCreateReadbackEvidence>(
        const vdrsuite::timers::NativeTimerCreateReadbackExpectation&)>;

struct NativeTimerCreateProductiveRuntimeSummary
{
    std::size_t discovered = 0;
    std::size_t dispatchClaims = 0;
    std::size_t activations = 0;
    std::size_t outcomesApplied = 0;
    std::size_t reconciliationsCompleted = 0;
    std::size_t deferred = 0;
};

// Advances only already-durable Timer CREATE lifecycle state. The caller owns
// cadence and authoritative native reads. This function owns no persistence,
// polling loop, retry loop or native transport.
NativeTimerCreateProductiveRuntimeSummary
advanceNativeTimerCreateRuntimeOnce(
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    BackendAgentCommandRepository& commandRepository,
    vdrsuite::agent::BackendAgentNativeTimerCreateReservationService&
        reservationService,
    vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService,
    vdrsuite::agent::BackendAgentNativeTimerCreateActivationService&
        activationService,
    vdrsuite::timers::NativeTimerCreateReadbackVerificationService&
        verificationService,
    vdrsuite::timers::TimerAssignmentFulfillmentService& fulfillmentService,
    vdrsuite::timers::NativeTimerCreateOperationCompletionService&
        completionService,
    const NativeTimerCreateReadbackAcquirer& readbackAcquirer,
    std::int64_t now);

} // namespace vdrsuite::daemon
