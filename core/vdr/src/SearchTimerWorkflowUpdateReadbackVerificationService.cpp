#include "SearchTimerWorkflowUpdateReadbackVerificationService.h"

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

bool identityMatches(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    return expected.backendId() == observed.backendId()
        && expected.backendNativeId() == observed.backendNativeId();
}

bool contentMatches(
    const SearchTimer& expected,
    const SearchTimer& observed)
{
    return expected.name() == observed.name()
        && expected.query() == observed.query()
        && expected.state() == observed.state();
}

bool exactUpdateReadbackMatch(
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
        if (exactUpdateReadbackMatch(expected, candidate))
        {
            matches.push_back(candidate);
        }
    }

    return matches;
}

int sameIdentityCount(
    const SearchTimer& expected,
    const SearchTimerResult& readback)
{
    int count = 0;

    for (const SearchTimer& candidate : readback.items())
    {
        if (identityMatches(expected, candidate))
        {
            ++count;
        }
    }

    return count;
}

std::string expectedNativeIdFor(
    const SearchTimerUpdateResult& updateResult)
{
    return updateResult.searchTimer.backendNativeId();
}

std::string expectedBackendIdFor(
    const SearchTimerUpdateResult& updateResult)
{
    return updateResult.searchTimer.backendId();
}

}

SearchTimerWorkflowBackendReadbackVerificationResult
SearchTimerWorkflowUpdateReadbackVerificationService::verify(
    const SearchTimerUpdateResult& updateResult,
    const ISearchTimerDataSource* dataSource) const
{
    const std::string expectedBackendId =
        expectedBackendIdFor(updateResult);
    const std::string expectedBackendNativeId =
        expectedNativeIdFor(updateResult);

    if (!updateResult.success)
    {
        std::vector<std::string> errors = updateResult.errors;

        if (errors.empty())
        {
            errors.push_back(
                "update result was not successful");
        }

        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                expectedBackendId,
                expectedBackendNativeId,
                "update result was not successful; backend readback skipped",
                errors);
        result.auditTrail.push_back("updateResultSuccess=false");
        return result;
    }

    const SearchTimer& expected =
        updateResult.searchTimer;

    if (!hasNativeId(expected))
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                expectedBackendId,
                expectedBackendNativeId,
                "updated SearchTimer is missing backend-native id",
                {"updated SearchTimer is missing backend-native id"});
        result.auditTrail.push_back("expectedBackendNativeIdAvailable=false");
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
                "updated SearchTimer state was not verified by backend readback",
                {"updated SearchTimer state was not verified by backend readback"});
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("sameIdentityCount=" + std::to_string(sameIdentityCount(expected, readback)));
        result.auditTrail.push_back("exactMatchCount=0");
        return result;
    }

    if (matches.size() > 1)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::ambiguousMatch(
                expectedBackendId,
                expectedBackendNativeId,
                "backend readback found multiple matching SearchTimers after update",
                {"backend readback returned multiple matching SearchTimers after update"},
                {"backend readback ambiguity requires operator review"});
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("sameIdentityCount=" + std::to_string(sameIdentityCount(expected, readback)));
        result.auditTrail.push_back("exactMatchCount=" + std::to_string(matches.size()));
        return result;
    }

    SearchTimerWorkflowBackendReadbackVerificationResult result =
        SearchTimerWorkflowBackendReadbackVerificationResult::verified(
            expectedBackendId,
            expectedBackendNativeId,
            matches.front().backendNativeId(),
            "updated SearchTimer verified by backend readback");
    result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
    result.auditTrail.push_back("sameIdentityCount=" + std::to_string(sameIdentityCount(expected, readback)));
    result.auditTrail.push_back("exactMatchCount=1");
    return result;
}
