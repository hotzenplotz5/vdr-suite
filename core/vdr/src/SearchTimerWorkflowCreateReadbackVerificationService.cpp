#include "SearchTimerWorkflowCreateReadbackVerificationService.h"

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <string>
#include <vector>

namespace
{

bool hasNativeId(
    const SearchTimer& timer)
{
    return !timer.backendNativeId().empty();
}

bool stateMatches(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    return expected.state() == observed.state();
}

bool identityMatches(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    if (expected.backendId() != observed.backendId())
    {
        return false;
    }

    if (hasNativeId(expected) &&
        expected.backendNativeId() != observed.backendNativeId())
    {
        return false;
    }

    return true;
}

bool contentMatches(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    return expected.name() == observed.name()
        && expected.query() == observed.query()
        && stateMatches(expected, observed);
}

bool exactCreateReadbackMatch(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    return identityMatches(expected, observed)
        && contentMatches(expected, observed);
}

SearchTimerQuery readbackQueryFor(
    const SearchTimer& expected)
{
    SearchTimerQuery query =
        SearchTimerQuery::byBackend(
            expected.backendId());

    if (!expected.query().empty())
    {
        query.withText(
            expected.query());
    }
    else if (!expected.name().empty())
    {
        query.withText(
            expected.name());
    }

    query.withLimit(100, 0);

    return query;
}

std::vector<SearchTimer> exactMatches(
    const SearchTimer& expected,
    const SearchTimerResult& readback)
{
    std::vector<SearchTimer> matches;

    for (const SearchTimer& candidate : readback.items())
    {
        if (exactCreateReadbackMatch(expected, candidate))
        {
            matches.push_back(candidate);
        }
    }

    return matches;
}

std::string expectedNativeIdFor(
    const SearchTimerCreateResult& createResult)
{
    return createResult.searchTimer.backendNativeId();
}

std::string expectedBackendIdFor(
    const SearchTimerCreateResult& createResult)
{
    return createResult.searchTimer.backendId();
}

}

SearchTimerWorkflowBackendReadbackVerificationResult
SearchTimerWorkflowCreateReadbackVerificationService::verify(
    const SearchTimerCreateResult& createResult,
    const ISearchTimerDataSource* dataSource) const
{
    const std::string expectedBackendId =
        expectedBackendIdFor(createResult);
    const std::string expectedBackendNativeId =
        expectedNativeIdFor(createResult);

    if (!createResult.success)
    {
        std::vector<std::string> errors = createResult.errors;

        if (errors.empty())
        {
            errors.push_back(
                "create result was not successful");
        }

        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                expectedBackendId,
                expectedBackendNativeId,
                "create result was not successful; backend readback skipped",
                errors);
        result.auditTrail.push_back("createResultSuccess=false");
        return result;
    }

    if (dataSource == nullptr)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                expectedBackendId,
                expectedBackendNativeId,
                "backend readback data source unavailable",
                {"backend readback data source unavailable"});
        result.auditTrail.push_back("dataSourceAvailable=false");
        return result;
    }

    const SearchTimer& expected =
        createResult.searchTimer;

    const SearchTimerResult readback =
        dataSource->list(
            readbackQueryFor(expected));

    const std::vector<SearchTimer> matches =
        exactMatches(
            expected,
            readback);

    if (matches.empty())
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::failed(
                expectedBackendId,
                expectedBackendNativeId,
                "",
                "created SearchTimer was not found by backend readback",
                {"created SearchTimer was not found by backend readback"});
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("exactMatchCount=0");
        return result;
    }

    if (matches.size() > 1)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::ambiguousMatch(
                expectedBackendId,
                expectedBackendNativeId,
                "backend readback found multiple matching SearchTimers after create",
                {"backend readback returned multiple matching SearchTimers after create"},
                {"backend readback ambiguity requires operator review"});
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("exactMatchCount=" + std::to_string(matches.size()));
        return result;
    }

    SearchTimerWorkflowBackendReadbackVerificationResult result =
        SearchTimerWorkflowBackendReadbackVerificationResult::verified(
            expectedBackendId,
            expectedBackendNativeId,
            matches.front().backendNativeId(),
            "created SearchTimer verified by backend readback");
    result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
    result.auditTrail.push_back("exactMatchCount=1");
    return result;
}
