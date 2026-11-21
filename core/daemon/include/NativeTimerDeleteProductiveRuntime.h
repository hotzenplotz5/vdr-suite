#pragma once

#include "NativeTimerInventoryEvidence.h"
#include <cstdint>
#include <functional>
#include <optional>

class BackendAgentCommandRepository;
class BackendAgentRepository;
namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::agent { class BackendAgentNativeTimerDeleteAssignmentService; }
namespace vdrsuite::timers {
class NativeTimerBindingRepository;
class NativeTimerDeleteDispatchService;
class NativeTimerAbsenceReadbackVerificationService;
class NativeTimerDeleteOperationCompletionService;
struct NativeTimerAbsenceReadbackExpectation;
}

namespace vdrsuite::daemon {
using NativeDeleteReadbackAcquirer =
    std::function<std::optional<vdrsuite::timers::NativeTimerInventoryEvidence>(
        const vdrsuite::timers::NativeTimerAbsenceReadbackExpectation&)>;

struct NativeTimerDeleteProductiveRuntimeSummary {
    std::size_t discovered = 0;
    std::size_t dispatched = 0;
    std::size_t outcomes = 0;
    std::size_t completed = 0;
    std::size_t deferred = 0;
};

// One bounded daemon iteration. Never retries the native side effect itself;
// outcome_unknown requires full native inventory and reconciling only.
NativeTimerDeleteProductiveRuntimeSummary advanceNativeTimerDeleteRuntimeOnce(
    vdrsuite::operations::MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    vdrsuite::timers::NativeTimerBindingRepository& bindings,
    vdrsuite::agent::BackendAgentNativeTimerDeleteAssignmentService& assignment,
    vdrsuite::timers::NativeTimerDeleteDispatchService& dispatch,
    vdrsuite::timers::NativeTimerAbsenceReadbackVerificationService& verification,
    vdrsuite::timers::NativeTimerDeleteOperationCompletionService& completion,
    const NativeDeleteReadbackAcquirer& readback,
    std::int64_t now);
}
