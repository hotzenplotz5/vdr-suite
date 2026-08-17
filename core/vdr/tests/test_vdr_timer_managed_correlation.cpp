#include "VdrTimerManagedCorrelation.h"

#include <cassert>
#include <iostream>
#include <string>

int main()
{
    const VdrTimerManagedCorrelation correlation{
        "assignment-1",
        "binding-1"
    };
    assert(vdrTimerManagedCorrelationValid(correlation));

    const std::string foreignAux =
        "<epgsearch><searchtimer>42</searchtimer></epgsearch>";
    assert(parseVdrTimerManagedCorrelation(foreignAux).status
        == VdrTimerManagedCorrelationStatus::absent);

    const auto attached =
        attachVdrTimerManagedCorrelation(foreignAux, correlation);
    assert(attached.ok());
    assert(attached.aux.compare(0, foreignAux.size(), foreignAux) == 0);

    const auto parsed = parseVdrTimerManagedCorrelation(attached.aux);
    assert(parsed.ok());
    assert(parsed.correlation.timerAssignmentId == correlation.timerAssignmentId);
    assert(parsed.correlation.nativeTimerBindingId
        == correlation.nativeTimerBindingId);

    const auto repeated =
        attachVdrTimerManagedCorrelation(attached.aux, correlation);
    assert(repeated.ok());
    assert(repeated.aux == attached.aux);

    VdrTimerManagedCorrelation other = correlation;
    other.nativeTimerBindingId = "binding-2";
    const auto conflict = attachVdrTimerManagedCorrelation(attached.aux, other);
    assert(conflict.status == VdrTimerManagedCorrelationStatus::conflictingMarker);
    assert(conflict.aux == attached.aux);

    const std::string malformed = "<vdr-suite-managed-timer-broken/>";
    assert(parseVdrTimerManagedCorrelation(malformed).status
        == VdrTimerManagedCorrelationStatus::malformedMarker);
    const auto malformedAttach =
        attachVdrTimerManagedCorrelation(malformed, correlation);
    assert(malformedAttach.status
        == VdrTimerManagedCorrelationStatus::malformedMarker);
    assert(malformedAttach.aux == malformed);

    const std::string duplicate = attached.aux + attached.aux;
    assert(parseVdrTimerManagedCorrelation(duplicate).status
        == VdrTimerManagedCorrelationStatus::conflictingMarker);

    std::cout << "test_vdr_timer_managed_correlation passed\n";
    return 0;
}
