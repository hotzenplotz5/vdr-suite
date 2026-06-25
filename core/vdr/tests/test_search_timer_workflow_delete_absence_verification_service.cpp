#include "SearchTimerWorkflowDeleteAbsenceVerificationService.h"

#include "ISearchTimerDataSource.h"
#include "SearchTimer.h"
#include "SearchTimerDeleteResult.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "SearchTimerService.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{

SearchTimer makeTimer(
    const std::string& backendId,
    const std::string& nativeId,
    const std::string& name,
    const std::string& query,
    SearchTimerState state)
{
    return SearchTimer::create(
        SearchTimerId::fromBackendNativeId(
            backendId,
            nativeId),
        name,
        query,
        state);
}

class StaticSearchTimerDataSource final : public ISearchTimerDataSource
{
public:
    explicit StaticSearchTimerDataSource(
        std::vector<SearchTimer> timers)
        : timers_(std::move(timers))
    {
    }

    SearchTimerResult list(
        const SearchTimerQuery& query) const override
    {
        return service_.list(
            timers_,
            query);
    }

private:
    std::vector<SearchTimer> timers_;
    SearchTimerService service_;
};

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
    SearchTimerWorkflowDeleteAbsenceVerificationService verificationService;

    const SearchTimerDeleteResult successfulDelete =
        SearchTimerDeleteResult::ok(
            "home-vdr",
            "native-42",
            "deleted");

    StaticSearchTimerDataSource absenceDataSource({
        makeTimer(
            "home-vdr",
            "native-99",
            "Other SearchTimer",
            "Other",
            SearchTimerState::Active),
        makeTimer(
            "remote-vdr",
            "native-42",
            "Same native id on other backend",
            "Other",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult verified =
        verificationService.verify(
            successfulDelete,
            &absenceDataSource);

    assert(verified.required);
    assert(verified.attempted);
    assert(verified.successful);
    assert(verified.matched);
    assert(!verified.ambiguous);
    assert(verified.passed());
    assert(verified.expectedBackendId == "home-vdr");
    assert(verified.expectedBackendNativeId == "native-42");
    assert(verified.observedBackendNativeId.empty());
    assert(contains(verified.auditTrail, "sameNativeIdCount=0"));
    assert(contains(verified.auditTrail, "absenceVerified=true"));

    StaticSearchTimerDataSource stillVisibleDataSource({
        makeTimer(
            "home-vdr",
            "native-42",
            "Deleted SearchTimer",
            "Terra X",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult stillVisible =
        verificationService.verify(
            successfulDelete,
            &stillVisibleDataSource);

    assert(stillVisible.required);
    assert(stillVisible.attempted);
    assert(!stillVisible.successful);
    assert(!stillVisible.matched);
    assert(!stillVisible.ambiguous);
    assert(!stillVisible.passed());
    assert(stillVisible.hasErrors());
    assert(stillVisible.observedBackendNativeId == "native-42");
    assert(contains(stillVisible.auditTrail, "sameNativeIdCount=1"));
    assert(contains(stillVisible.auditTrail, "absenceVerified=false"));

    StaticSearchTimerDataSource ambiguousDataSource({
        makeTimer(
            "home-vdr",
            "native-42",
            "Deleted SearchTimer A",
            "Terra X",
            SearchTimerState::Active),
        makeTimer(
            "home-vdr",
            "native-42",
            "Deleted SearchTimer B",
            "Terra X",
            SearchTimerState::Inactive)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult ambiguous =
        verificationService.verify(
            successfulDelete,
            &ambiguousDataSource);

    assert(ambiguous.required);
    assert(ambiguous.attempted);
    assert(!ambiguous.successful);
    assert(!ambiguous.matched);
    assert(ambiguous.ambiguous);
    assert(!ambiguous.passed());
    assert(ambiguous.hasWarnings());
    assert(ambiguous.hasErrors());
    assert(ambiguous.observedBackendNativeId == "native-42");
    assert(contains(ambiguous.auditTrail, "sameNativeIdCount=2"));
    assert(contains(ambiguous.auditTrail, "absenceVerified=false"));

    const SearchTimerWorkflowBackendReadbackVerificationResult unavailable =
        verificationService.verify(
            successfulDelete,
            nullptr);

    assert(unavailable.required);
    assert(!unavailable.attempted);
    assert(!unavailable.successful);
    assert(!unavailable.matched);
    assert(!unavailable.passed());
    assert(unavailable.hasErrors());
    assert(contains(unavailable.auditTrail, "dataSourceAvailable=false"));

    const SearchTimerDeleteResult missingBackendId =
        SearchTimerDeleteResult::ok(
            "",
            "native-42",
            "deleted");

    const SearchTimerWorkflowBackendReadbackVerificationResult missingBackend =
        verificationService.verify(
            missingBackendId,
            &absenceDataSource);

    assert(missingBackend.required);
    assert(!missingBackend.attempted);
    assert(!missingBackend.successful);
    assert(!missingBackend.matched);
    assert(!missingBackend.passed());
    assert(missingBackend.hasErrors());
    assert(contains(missingBackend.auditTrail, "expectedBackendIdAvailable=false"));

    const SearchTimerDeleteResult missingNativeId =
        SearchTimerDeleteResult::ok(
            "home-vdr",
            "",
            "deleted");

    const SearchTimerWorkflowBackendReadbackVerificationResult missingNative =
        verificationService.verify(
            missingNativeId,
            &absenceDataSource);

    assert(missingNative.required);
    assert(!missingNative.attempted);
    assert(!missingNative.successful);
    assert(!missingNative.matched);
    assert(!missingNative.passed());
    assert(missingNative.hasErrors());
    assert(contains(missingNative.auditTrail, "expectedBackendNativeIdAvailable=false"));

    const SearchTimerDeleteResult failedDelete =
        SearchTimerDeleteResult::failed(
            "delete failed",
            {"backend delete failed"});

    const SearchTimerWorkflowBackendReadbackVerificationResult skipped =
        verificationService.verify(
            failedDelete,
            &absenceDataSource);

    assert(skipped.required);
    assert(!skipped.attempted);
    assert(!skipped.successful);
    assert(!skipped.matched);
    assert(!skipped.passed());
    assert(skipped.hasErrors());
    assert(contains(skipped.auditTrail, "deleteResultSuccess=false"));

    std::cout
        << "test_search_timer_workflow_delete_absence_verification_service passed"
        << std::endl;

    return 0;
}
