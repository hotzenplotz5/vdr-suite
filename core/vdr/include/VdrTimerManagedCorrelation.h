#pragma once

#include <string>

struct VdrTimerManagedCorrelation
{
    std::string timerAssignmentId;
    std::string nativeTimerBindingId;
};

enum class VdrTimerManagedCorrelationStatus
{
    ok,
    absent,
    invalidCorrelation,
    malformedMarker,
    conflictingMarker,
};

struct VdrTimerManagedCorrelationParseResult
{
    VdrTimerManagedCorrelationStatus status =
        VdrTimerManagedCorrelationStatus::absent;
    VdrTimerManagedCorrelation correlation;

    bool ok() const
    {
        return status == VdrTimerManagedCorrelationStatus::ok;
    }
};

struct VdrTimerManagedCorrelationAttachResult
{
    VdrTimerManagedCorrelationStatus status =
        VdrTimerManagedCorrelationStatus::invalidCorrelation;
    std::string aux;

    bool ok() const
    {
        return status == VdrTimerManagedCorrelationStatus::ok;
    }
};

bool vdrTimerManagedCorrelationValid(
    const VdrTimerManagedCorrelation& correlation);
VdrTimerManagedCorrelationParseResult parseVdrTimerManagedCorrelation(
    const std::string& aux);
VdrTimerManagedCorrelationAttachResult attachVdrTimerManagedCorrelation(
    const std::string& aux,
    const VdrTimerManagedCorrelation& correlation);
