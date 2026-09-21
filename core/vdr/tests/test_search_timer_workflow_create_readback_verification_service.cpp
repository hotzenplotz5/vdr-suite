#include "SearchTimerWorkflowCreateReadbackVerificationService.h"

#include "ISearchTimerDataSource.h"
#include "SearchTimerCreateResult.h"
#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"
#include "SearchTimerService.h"

#include <cassert>
#include <iostream>
#include <string>
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
    SearchTimerWorkflowCreateReadbackVerificationService verificationService;

    const SearchTimer expected =
        makeTimer(
            "home-vdr",
            "native-42",
            "Terra X Suche",
            "Terra X",
            SearchTimerState::Active);

    const SearchTimerCreateResult successfulCreate =
        SearchTimerCreateResult::ok(
            expected,
            "created");

    StaticSearchTimerDataSource matchingDataSource({
        expected
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult verified =
        verificationService.verify(
            successfulCreate,
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
    assert(contains(verified.auditTrail, "exactMatchCount=1"));

    const SearchTimerWorkflowBackendReadbackVerificationResult unavailable =
        verificationService.verify(
            successfulCreate,
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
            successfulCreate,
            &emptyDataSource);

    assert(missing.required);
    assert(missing.attempted);
    assert(!missing.successful);
    assert(!missing.matched);
    assert(!missing.ambiguous);
    assert(!missing.passed());
    assert(missing.hasErrors());
    assert(contains(missing.auditTrail, "exactMatchCount=0"));

    StaticSearchTimerDataSource wrongNativeIdDataSource({
        makeTimer(
            "home-vdr",
            "native-99",
            "Terra X Suche",
            "Terra X",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult wrongNativeId =
        verificationService.verify(
            successfulCreate,
            &wrongNativeIdDataSource);

    assert(wrongNativeId.required);
    assert(wrongNativeId.attempted);
    assert(!wrongNativeId.successful);
    assert(!wrongNativeId.matched);
    assert(!wrongNativeId.passed());
    assert(contains(wrongNativeId.auditTrail, "exactMatchCount=0"));

    const SearchTimer expectedWithoutNativeId =
        makeTimer(
            "home-vdr",
            "",
            "Terra X Suche",
            "Terra X",
            SearchTimerState::Active);

    const SearchTimerCreateResult createWithoutNativeId =
        SearchTimerCreateResult::ok(
            expectedWithoutNativeId,
            "created");

    StaticSearchTimerDataSource ambiguousDataSource({
        makeTimer(
            "home-vdr",
            "native-1",
            "Terra X Suche",
            "Terra X",
            SearchTimerState::Active),
        makeTimer(
            "home-vdr",
            "native-2",
            "Terra X Suche",
            "Terra X",
            SearchTimerState::Active)
    });

    const SearchTimerWorkflowBackendReadbackVerificationResult ambiguous =
        verificationService.verify(
            createWithoutNativeId,
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

    const SearchTimerCreateResult failedCreate =
        SearchTimerCreateResult::failed(
            "create failed",
            {"backend create failed"});

    const SearchTimerWorkflowBackendReadbackVerificationResult skipped =
        verificationService.verify(
            failedCreate,
            &matchingDataSource);

    assert(skipped.required);
    assert(!skipped.attempted);
    assert(!skipped.successful);
    assert(!skipped.matched);
    assert(!skipped.passed());
    assert(skipped.hasErrors());
    assert(contains(skipped.auditTrail, "createResultSuccess=false"));

    std::cout
        << "test_search_timer_workflow_create_readback_verification_service passed"
        << std::endl;

    return 0;
}
