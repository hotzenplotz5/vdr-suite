#pragma once

#include "NativeTimerObservation.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

class BackendAgentCommandRepository;
class BackendAgentRepository;
namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::agent { class BackendAgentNativeTimerModifyAssignmentService; }
namespace vdrsuite::timers {
class NativeTimerBindingRepository;
class TimerIntentRepository;
class TimerAssignmentRepository;
class NativeTimerModifyDispatchService;
class NativeTimerModifyReadbackVerificationService;
class NativeTimerModifyOperationCompletionService;
struct NativeTimerModifyReadbackExpectation;
}

namespace vdrsuite::daemon {
using NativeModifyReadbackAcquirer = std::function<
    std::optional<vdrsuite::timers::NativeTimerObservation>(
        const vdrsuite::timers::NativeTimerModifyReadbackExpectation&)>;
struct NativeTimerModifyProductiveSummary {
    std::size_t discovered = 0;
    std::size_t claimed = 0;
    std::size_t applied = 0;
    std::size_t completed = 0;
    std::size_t deferred = 0;
};

NativeTimerModifyProductiveSummary advanceNativeTimerModifyRuntimeOnce(
    vdrsuite::operations::MutationOperationRepository& operations,
    BackendAgentCommandRepository& commands,
    BackendAgentRepository& agents,
    vdrsuite::timers::TimerIntentRepository& intents,
    vdrsuite::timers::TimerAssignmentRepository& assignments,
    vdrsuite::timers::NativeTimerBindingRepository& bindings,
    vdrsuite::agent::BackendAgentNativeTimerModifyAssignmentService& assigner,
    vdrsuite::timers::NativeTimerModifyDispatchService& dispatcher,
    vdrsuite::timers::NativeTimerModifyReadbackVerificationService& verifier,
    vdrsuite::timers::NativeTimerModifyOperationCompletionService& completion,
    const NativeModifyReadbackAcquirer& readback,
    std::int64_t now);
}
