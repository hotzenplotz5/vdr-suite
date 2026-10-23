#pragma once

#include "NativeTimerModifyDispatchService.h"

#include <string>

class BackendAgentCommandRepository;
namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::timers { class NativeTimerBindingRepository; }

namespace vdrsuite::daemon
{
enum class NativeTimerModifyResultStatus {
    applied, alreadyApplied, unavailable, missingResult,
    invalidEvidence, identityConflict, revisionConflict,
    operationConflict
};
struct NativeTimerModifyResultApplication {
    NativeTimerModifyResultStatus status = NativeTimerModifyResultStatus::unavailable;
    vdrsuite::timers::NativeTimerModifyDispatchOutcomeResult outcome;
    bool ok() const {
        return status == NativeTimerModifyResultStatus::applied ||
            status == NativeTimerModifyResultStatus::alreadyApplied;
    }
};

// The only supported bridge from a durable Agent UPDATE/TOGGLE result
// into the Phase-64 mutation state machine. This does not imply readback.
NativeTimerModifyResultApplication applyDurableNativeTimerModifyResult(
    BackendAgentCommandRepository& commands,
    vdrsuite::operations::MutationOperationRepository& operations,
    vdrsuite::timers::NativeTimerBindingRepository& bindings,
    vdrsuite::timers::NativeTimerModifyDispatchService& dispatcher,
    const std::string& commandId);
}
