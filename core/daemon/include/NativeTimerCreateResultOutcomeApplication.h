#pragma once

#include "NativeTimerCreateDispatchService.h"

#include <string>

class BackendAgentCommandRepository;

namespace vdrsuite::operations
{
class MutationOperationRepository;
}

namespace vdrsuite::daemon
{

enum class NativeTimerCreateResultOutcomeApplicationStatus
{
    applied,
    alreadyApplied,
    assignmentNotFound,
    resultNotFound,
    commandTypeConflict,
    evidenceInvalid,
    operationNotFound,
    operationRepositoryError,
    operationStateConflict,
    operationRevisionConflict,
    dispatchRejected,
};

struct NativeTimerCreateResultOutcomeApplicationResult
{
    NativeTimerCreateResultOutcomeApplicationStatus status =
        NativeTimerCreateResultOutcomeApplicationStatus::operationRepositoryError;
    vdrsuite::timers::NativeTimerCreateDispatchOutcomeResult dispatch;
    std::string reasonCode;

    bool ok() const
    {
        return status == NativeTimerCreateResultOutcomeApplicationStatus::applied ||
            status == NativeTimerCreateResultOutcomeApplicationStatus::alreadyApplied;
    }
};

// Stateless daemon-composition adapter. It reads only the existing durable
// command/result and MutationOperation authorities, then delegates the state
// transition to NativeTimerCreateDispatchService. It owns no persistence and
// does not make a command pollable.
NativeTimerCreateResultOutcomeApplicationResult
applyDurableNativeTimerCreateResult(
    BackendAgentCommandRepository& commandRepository,
    vdrsuite::operations::MutationOperationRepository& operationRepository,
    vdrsuite::timers::NativeTimerCreateDispatchService& dispatchService,
    const std::string& commandId);

} // namespace vdrsuite::daemon
