#pragma once

#include "NativeTimerBinding.h"
#include "NativeTimerInventoryEvidence.h"

#include <string>

namespace vdrsuite::timers
{

class NativeTimerBindingRepository;

enum class NativeTimerBindingAbsenceApplicationStatus
{
    missingRecorded,
    missingRefreshed,
    alreadyCurrent,
    present,
    reconciliationRequired,
    bindingNotFound,
    backendConflict,
    staleGeneration,
    staleEvidence,
    repositoryConflict,
    repositoryError,
    invalid,
};

struct NativeTimerBindingAbsenceApplicationResult
{
    NativeTimerBindingAbsenceApplicationStatus status =
        NativeTimerBindingAbsenceApplicationStatus::repositoryError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerBindingAbsenceApplicationStatus::missingRecorded
            || status == NativeTimerBindingAbsenceApplicationStatus::missingRefreshed
            || status == NativeTimerBindingAbsenceApplicationStatus::alreadyCurrent
            || status == NativeTimerBindingAbsenceApplicationStatus::present;
    }
};

class NativeTimerBindingAbsenceApplicationService
{
public:
    explicit NativeTimerBindingAbsenceApplicationService(
        NativeTimerBindingRepository& repository);

    NativeTimerBindingAbsenceApplicationResult apply(
        const std::string& nativeTimerBindingId,
        const NativeTimerInventoryEvidence& evidence);

private:
    NativeTimerBindingRepository& repository_;
};

}
