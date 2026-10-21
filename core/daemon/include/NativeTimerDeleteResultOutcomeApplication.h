#pragma once

#include "NativeTimerDeleteDispatchService.h"

#include <string>

class BackendAgentCommandRepository;

namespace vdrsuite::operations { class MutationOperationRepository; }
namespace vdrsuite::timers { class NativeTimerBindingRepository; }

namespace vdrsuite::daemon
{

enum class NativeTimerDeleteResultOutcomeStatus
{
    applied,
    alreadyApplied,
    missingCommand,
    missingResult,
    invalidEvidence,
    identityConflict,
    revisionConflict,
    operationConflict,
    unavailable,
};

struct NativeTimerDeleteResultOutcomeResult
{
    NativeTimerDeleteResultOutcomeStatus status =
        NativeTimerDeleteResultOutcomeStatus::unavailable;
    std::string reasonCode;
    vdrsuite::timers::NativeTimerDeleteDispatchOutcomeResult dispatch;
    bool ok() const
    {
        return status == NativeTimerDeleteResultOutcomeStatus::applied ||
            status == NativeTimerDeleteResultOutcomeStatus::alreadyApplied;
    }
};

// Process only a durable Agent result with a canonical, identity-fenced
// execution boundary. Never infer native dispatch from a generic command
// outcome category. This does not itself verify VDR absence.
NativeTimerDeleteResultOutcomeResult applyDurableNativeTimerDeleteResult(
    BackendAgentCommandRepository& commandRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerBindingRepository& bindingRepository,
    vdrsuite::timers::NativeTimerDeleteDispatchService& dispatchService,
    const std::string& commandId);

}
