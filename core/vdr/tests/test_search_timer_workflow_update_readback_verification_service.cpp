#include "SearchTimerWorkflowUpdateReadbackVerificationService.h"

#include "ISearchTimerDataSource.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "SearchTimerService.h"
#include "SearchTimerUpdateResult.h"

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
    SearchTimerWorkflowUpdateReadbackVerificationService verificationService;

    const SearchTimer expected =
        makeTimer(
            "home-vdr",
            "native-42",
            "Terra X Suche aktualisiert",
            "Terra X Expedition",
            SearchTimerState::Inactive);

    const SearchTimerUpdateResult successfulUpdate =
        SearchTimerUpdateResult::ok(
            expected,
            "updated");

    StaticSearchTimerDataSource matchingDataSource({
        expected
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult verified =
        verificationService.verify(
            successfulUpdate,
            &matchingDataSource);

    assert(verified.required);
    assert(verified.attempted);
    assert(verified.successful);
    assert(verified.matched);
    assert(!verified.ambiguous);
    assert(verified.passed());
    assert(verified.expectedBackendId == "home-vdr");
    assert(verified.expectedBackendNativeId == "native-42");
    assert(verified.observedBackendNativeId == "native-42");
    assert(contains(verified.auditTrail, "sameIdentityCount=1"));
    assert(contains(verified.auditTrail, "exactMatchCount=1"));

    const SearchTimerWorkflowBackendReadbackVerificationResult unavailable =
        verificationService.verify(
            successfulUpdate,
            nullptr);

    assert(unavailable.required);
    assert(!unavailable.attempted);
    assert(!unavailable.successful);
    assert(!unavailable.matched);
    assert(!unavailable.passed());
    assert(unavailable.hasErrors());
    assert(contains(unavailable.auditTrail, "dataSourceAvailable=false"));

    StaticSearchTimerDataSource emptyDataSource({});

    const SearchTimerWorkflowBackendReadbackVerificationResult missing =
        verificationService.verify(
            successfulUpdate,
            &emptyDataSource);

    assert(missing.required);
    assert(missing.attempted);
    assert(!missing.successful);
    assert(!missing.matched);
    assert(!missing.ambiguous);
    assert(!missing.passed());
    assert(missing.hasErrors());
    assert(contains(missing.auditTrail, "sameIdentityCount=0"));
    assert(contains(missing.auditTrail, "exactMatchCount=0"));

    StaticSearchTimerDataSource unchangedDataSource({
        makeTimer(
            "home-vdr",
            "native-42",
            "Terra X Suche alt",
            "Terra X",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult unchanged =
        verificationService.verify(
            successfulUpdate,
            &unchangedDataSource);

    assert(unchanged.required);
    assert(unchanged.attempted);
    assert(!unchanged.successful);
    assert(!unchanged.matched);
    assert(!unchanged.ambiguous);
    assert(!unchanged.passed());
    assert(unchanged.hasErrors());
    assert(contains(unchanged.auditTrail, "sameIdentityCount=1"));
    assert(contains(unchanged.auditTrail, "exactMatchCount=0"));

    StaticSearchTimerDataSource wrongNativeIdDataSource({
        makeTimer(
            "home-vdr",
            "native-99",
            "Terra X Suche aktualisiert",
            "Terra X Expedition",
            SearchTimerState::Inactive)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult wrongNativeId =
        verificationService.verify(
            successfulUpdate,
            &wrongNativeIdDataSource);

    assert(wrongNativeId.required);
    assert(wrongNativeId.attempted);
    assert(!wrongNativeId.successful);
    assert(!wrongNativeId.matched);
    assert(!wrongNativeId.passed());
    assert(contains(wrongNativeId.auditTrail, "sameIdentityCount=0"));
    assert(contains(wrongNativeId.auditTrail, "exactMatchCount=0"));

    StaticSearchTimerDataSource ambiguousDataSource({
        expected,
        expected
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult ambiguous =
        verificationService.verify(
            successfulUpdate,
            &ambiguousDataSource);

    assert(ambiguous.required);
    assert(ambiguous.attempted);
    assert(!ambiguous.successful);
    assert(!ambiguous.matched);
    assert(ambiguous.ambiguous);
    assert(!ambiguous.passed());
    assert(ambiguous.hasWarnings());
    assert(ambiguous.hasErrors());
    assert(contains(ambiguous.auditTrail, "exactMatchCount=2"));

    const SearchTimerUpdateResult missingNativeIdUpdate =
        SearchTimerUpdateResult::ok(
            makeTimer(
                "home-vdr",
                "",
                "Terra X Suche aktualisiert",
                "Terra X Expedition",
                SearchTimerState::Inactive),
            "updated");

    const SearchTimerWorkflowBackendReadbackVerificationResult missingNativeId =
        verificationService.verify(
            missingNativeIdUpdate,
            &matchingDataSource);

    assert(missingNativeId.required);
    assert(!missingNativeId.attempted);
    assert(!missingNativeId.successful);
    assert(!missingNativeId.matched);
    assert(!missingNativeId.passed());
    assert(missingNativeId.hasErrors());
    assert(contains(missingNativeId.auditTrail, "expectedBackendNativeIdAvailable=false"));

    const SearchTimerUpdateResult failedUpdate =
        SearchTimerUpdateResult::failed(
            "update failed",
            {"backend update failed"});

    const SearchTimerWorkflowBackendReadbackVerificationResult skipped =
        verificationService.verify(
            failedUpdate,
            &matchingDataSource);

    assert(skipped.required);
    assert(!skipped.attempted);
    assert(!skipped.successful);
    assert(!skipped.matched);
    assert(!skipped.passed());
    assert(skipped.hasErrors());
    assert(contains(skipped.auditTrail, "updateResultSuccess=false"));

    std::cout
        << "test_search_timer_workflow_update_readback_verification_service passed"
        << std::endl;

    return 0;
}
