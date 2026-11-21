#include "NativeTimerInventoryEvidence.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::timers;

namespace
{
NativeTimerInventoryEvidence evidence()
{
    NativeTimerInventoryEvidence value;
    value.backendId = "backend:a";
    value.backendGeneration = 8;
    value.observedAt = 2200;
    value.completeness = NativeTimerInventoryCompleteness::complete;
    value.backendNativeTimerIds = {"timer:10", "timer:17", "timer:20"};
    return value;
}

NativeTimerAbsenceAssessmentRequest request(const std::string& nativeId)
{
    NativeTimerAbsenceAssessmentRequest value;
    value.backendId = "backend:a";
    value.backendGeneration = 8;
    value.backendNativeTimerId = nativeId;
    value.notBefore = 2000;
    return value;
}
}

int main()
{
    const auto complete = evidence();
    assert(nativeTimerInventoryEvidenceValid(complete));
    assert(assessNativeTimerAbsence(complete, request("timer:17")) ==
        NativeTimerAbsenceAssessmentStatus::present);
    assert(assessNativeTimerAbsence(complete, request("timer:18")) ==
        NativeTimerAbsenceAssessmentStatus::absent);

    auto empty = complete;
    empty.backendNativeTimerIds.clear();
    assert(nativeTimerInventoryEvidenceValid(empty));
    assert(assessNativeTimerAbsence(empty, request("timer:17")) ==
        NativeTimerAbsenceAssessmentStatus::absent);

    auto unknown = complete;
    unknown.completeness = NativeTimerInventoryCompleteness::unknown;
    assert(!nativeTimerInventoryEvidenceValid(unknown));
    assert(assessNativeTimerAbsence(unknown, request("timer:17")) ==
        NativeTimerAbsenceAssessmentStatus::invalid);

    auto duplicate = complete;
    duplicate.backendNativeTimerIds = {"timer:10", "timer:10"};
    assert(!nativeTimerInventoryEvidenceValid(duplicate));

    auto unsorted = complete;
    unsorted.backendNativeTimerIds = {"timer:20", "timer:10"};
    assert(!nativeTimerInventoryEvidenceValid(unsorted));

    auto wrongBackend = request("timer:17");
    wrongBackend.backendId = "backend:b";
    assert(assessNativeTimerAbsence(complete, wrongBackend) ==
        NativeTimerAbsenceAssessmentStatus::backendConflict);

    auto wrongGeneration = request("timer:17");
    wrongGeneration.backendGeneration = 9;
    assert(assessNativeTimerAbsence(complete, wrongGeneration) ==
        NativeTimerAbsenceAssessmentStatus::generationConflict);

    auto stale = complete;
    stale.observedAt = 1999;
    assert(assessNativeTimerAbsence(stale, request("timer:17")) ==
        NativeTimerAbsenceAssessmentStatus::staleEvidence);

    auto invalidRequest = request("timer:17");
    invalidRequest.notBefore = 0;
    assert(!nativeTimerAbsenceAssessmentRequestValid(invalidRequest));
    assert(assessNativeTimerAbsence(complete, invalidRequest) ==
        NativeTimerAbsenceAssessmentStatus::invalid);

    auto maxIdentity = complete;
    maxIdentity.backendNativeTimerIds = {std::string(160, 'a')};
    assert(nativeTimerInventoryEvidenceValid(maxIdentity));
    maxIdentity.backendNativeTimerIds = {std::string(161, 'a')};
    assert(!nativeTimerInventoryEvidenceValid(maxIdentity));

    auto tooMany = complete;
    tooMany.backendNativeTimerIds.clear();
    for (int i = 0; i < 4097; ++i)
    {
        std::string number = std::to_string(10000 + i);
        tooMany.backendNativeTimerIds.push_back("timer:" + number);
    }
    assert(!nativeTimerInventoryEvidenceValid(tooMany));

    std::cout << "test_native_timer_inventory_evidence passed\n";
    return 0;
}
