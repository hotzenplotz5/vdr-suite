#include "VdrManagedTimerCreateReadbackEvidenceBuilder.h"

#include "VdrNativeTimerObservationMapper.h"
#include "VdrTimerManagedCorrelation.h"

#include <algorithm>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
VdrManagedTimerCreateReadbackEvidenceBuildResult result(
    VdrManagedTimerCreateReadbackEvidenceBuildStatus status)
{
    VdrManagedTimerCreateReadbackEvidenceBuildResult value;
    value.status = status;
    return value;
}
}

VdrManagedTimerCreateReadbackEvidenceBuildResult
VdrManagedTimerCreateReadbackEvidenceBuilder::build(
    const vdrsuite::timers::NativeTimerInventoryEvidence& inventory,
    const std::vector<VdrTimer>& timers)
{
    using namespace vdrsuite::timers;

    if (!nativeTimerInventoryEvidenceValid(inventory))
    {
        return result(
            VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                invalidInventoryEvidence);
    }

    if (timers.size() != inventory.backendNativeTimerIds.size())
    {
        return result(
            VdrManagedTimerCreateReadbackEvidenceBuildStatus::inventoryMismatch);
    }

    NativeTimerCreateReadbackEvidence evidence;
    evidence.backendId = inventory.backendId;
    evidence.backendGeneration = inventory.backendGeneration;
    evidence.observedAt = inventory.observedAt;
    evidence.completeness = NativeTimerCreateReadbackCompleteness::complete;

    std::set<std::string> nativeIds;

    for (const auto& timer : timers)
    {
        const auto mapped = VdrNativeTimerObservationMapper::map(
            inventory.backendId,
            inventory.backendGeneration,
            inventory.observedAt,
            timer);
        if (!mapped.ok())
        {
            return result(
                VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                    invalidTimerObservation);
        }

        if (!nativeIds.insert(mapped.observation.backendNativeTimerId).second)
        {
            return result(
                VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                    duplicateNativeTimerIdentity);
        }

        const auto correlation = parseVdrTimerManagedCorrelation(timer.aux);
        if (correlation.status == VdrTimerManagedCorrelationStatus::absent)
            continue;
        if (correlation.status == VdrTimerManagedCorrelationStatus::malformedMarker
            || correlation.status == VdrTimerManagedCorrelationStatus::invalidCorrelation)
        {
            return result(
                VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                    malformedManagedCorrelation);
        }
        if (correlation.status == VdrTimerManagedCorrelationStatus::conflictingMarker)
        {
            return result(
                VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                    conflictingManagedCorrelation);
        }
        if (!correlation.ok())
        {
            return result(
                VdrManagedTimerCreateReadbackEvidenceBuildStatus::
                    malformedManagedCorrelation);
        }

        NativeTimerCreateReadbackCandidate candidate;
        candidate.timerAssignmentId = correlation.correlation.timerAssignmentId;
        candidate.nativeTimerBindingId = correlation.correlation.nativeTimerBindingId;
        candidate.observation = mapped.observation;
        evidence.candidates.push_back(std::move(candidate));
    }

    std::vector<std::string> observedIds(nativeIds.begin(), nativeIds.end());
    if (observedIds != inventory.backendNativeTimerIds)
    {
        return result(
            VdrManagedTimerCreateReadbackEvidenceBuildStatus::inventoryMismatch);
    }

    if (!nativeTimerCreateReadbackEvidenceValid(evidence))
    {
        return result(
            VdrManagedTimerCreateReadbackEvidenceBuildStatus::invalidEvidence);
    }

    VdrManagedTimerCreateReadbackEvidenceBuildResult value;
    value.status = VdrManagedTimerCreateReadbackEvidenceBuildStatus::ok;
    value.evidence = std::move(evidence);
    return value;
}
