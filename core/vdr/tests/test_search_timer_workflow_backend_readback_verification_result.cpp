#include "SearchTimerWorkflowBackendReadbackVerificationResult.h"

#include <cassert>
#include <iostream>
#include <string>

namespace
{

bool contains(
    const std::vector<std::string>& values,
    const std::string& expected)
{
    for (const std::string& value : values)
    {
        if (value == expected)
        {
            return true;
        }
    }

    return false;
}

}

int main()
{
    const SearchTimerWorkflowBackendReadbackVerificationResult notRequired =
        SearchTimerWorkflowBackendReadbackVerificationResult::notRequired();

    assert(!notRequired.required);
    assert(!notRequired.attempted);
    assert(notRequired.successful);
    assert(notRequired.matched);
    assert(!notRequired.ambiguous);
    assert(notRequired.passed());
    assert(!notRequired.hasWarnings());
    assert(!notRequired.hasErrors());
    assert(contains(notRequired.auditTrail, "required=false"));
    assert(contains(notRequired.auditTrail, "successful=true"));

    const SearchTimerWorkflowBackendReadbackVerificationResult unavailable =
        SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
            "home-vdr",
            "native-42",
            "backend readback data source unavailable",
            {"backend readback data source unavailable"});

    assert(unavailable.required);
    assert(!unavailable.attempted);
    assert(!unavailable.successful);
    assert(!unavailable.matched);
    assert(!unavailable.ambiguous);
    assert(!unavailable.passed());
    assert(unavailable.hasErrors());
    assert(unavailable.expectedBackendId == "home-vdr");
    assert(unavailable.expectedBackendNativeId == "native-42");
    assert(contains(unavailable.auditTrail, "attempted=false"));

    const SearchTimerWorkflowBackendReadbackVerificationResult verified =
        SearchTimerWorkflowBackendReadbackVerificationResult::verified(
            "home-vdr",
            "native-42",
            "native-42",
            "backend readback verified");

    assert(verified.required);
    assert(verified.attempted);
    assert(verified.successful);
    assert(verified.matched);
    assert(!verified.ambiguous);
    assert(verified.passed());
    assert(!verified.hasErrors());
    assert(verified.expectedBackendId == "home-vdr");
    assert(verified.expectedBackendNativeId == "native-42");
    assert(verified.observedBackendNativeId == "native-42");
    assert(contains(verified.auditTrail, "matched=true"));

    const SearchTimerWorkflowBackendReadbackVerificationResult failed =
        SearchTimerWorkflowBackendReadbackVerificationResult::failed(
            "home-vdr",
            "native-42",
            "native-99",
            "backend readback mismatch",
            {"backend-native id mismatch"});

    assert(failed.required);
    assert(failed.attempted);
    assert(!failed.successful);
    assert(!failed.matched);
    assert(!failed.ambiguous);
    assert(!failed.passed());
    assert(failed.hasErrors());
    assert(failed.expectedBackendNativeId == "native-42");
    assert(failed.observedBackendNativeId == "native-99");
    assert(contains(failed.auditTrail, "matched=false"));

    const SearchTimerWorkflowBackendReadbackVerificationResult ambiguous =
        SearchTimerWorkflowBackendReadbackVerificationResult::ambiguousMatch(
            "home-vdr",
            "",
            "backend readback found ambiguous matches",
            {"backend readback returned more than one candidate"},
            {"backend readback ambiguity requires operator review"});

    assert(ambiguous.required);
    assert(ambiguous.attempted);
    assert(!ambiguous.successful);
    assert(!ambiguous.matched);
    assert(ambiguous.ambiguous);
    assert(!ambiguous.passed());
    assert(ambiguous.hasWarnings());
    assert(ambiguous.hasErrors());
    assert(contains(ambiguous.auditTrail, "ambiguous=true"));

    std::cout
        << "test_search_timer_workflow_backend_readback_verification_result passed"
        << std::endl;

    return 0;
}
