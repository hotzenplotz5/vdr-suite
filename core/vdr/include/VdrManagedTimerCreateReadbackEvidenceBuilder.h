#pragma once

#include "NativeTimerCreateReadbackEvidence.h"
#include "NativeTimerInventoryEvidence.h"
#include "VdrTimer.h"

#include <vector>

enum class VdrManagedTimerCreateReadbackEvidenceBuildStatus
{
    ok,
    invalidInventoryEvidence,
    invalidTimerObservation,
    duplicateNativeTimerIdentity,
    inventoryMismatch,
    malformedManagedCorrelation,
    conflictingManagedCorrelation,
    invalidEvidence,
};

struct VdrManagedTimerCreateReadbackEvidenceBuildResult
{
    VdrManagedTimerCreateReadbackEvidenceBuildStatus status =
        VdrManagedTimerCreateReadbackEvidenceBuildStatus::invalidEvidence;
    vdrsuite::timers::NativeTimerCreateReadbackEvidence evidence;

    bool ok() const
    {
        return status == VdrManagedTimerCreateReadbackEvidenceBuildStatus::ok;
    }
};

class VdrManagedTimerCreateReadbackEvidenceBuilder
{
public:
    static VdrManagedTimerCreateReadbackEvidenceBuildResult build(
        const vdrsuite::timers::NativeTimerInventoryEvidence& inventory,
        const std::vector<VdrTimer>& timers);
};
