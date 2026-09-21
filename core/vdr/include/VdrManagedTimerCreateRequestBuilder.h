#pragma once

#include "NativeTimerSpecification.h"
#include "VdrTimerManagedCorrelation.h"
#include "VdrTimerOperationRequest.h"

#include <string>

enum class VdrManagedTimerCreateRequestBuildStatus
{
    ok,
    invalidBackendIdentity,
    invalidSpecification,
    invalidCorrelation,
    auxConflict,
};

struct VdrManagedTimerCreateRequestBuildResult
{
    VdrManagedTimerCreateRequestBuildStatus status =
        VdrManagedTimerCreateRequestBuildStatus::invalidSpecification;
    VdrTimerOperationRequest request;

    bool ok() const
    {
        return status == VdrManagedTimerCreateRequestBuildStatus::ok;
    }
};

class VdrManagedTimerCreateRequestBuilder
{
public:
    static VdrManagedTimerCreateRequestBuildResult build(
        const std::string& backendId,
        const vdrsuite::timers::NativeTimerSpecification& specification,
        const VdrTimerManagedCorrelation& correlation,
        const std::string& baseAux = {});
};
