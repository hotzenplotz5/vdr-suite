#include "SearchTimerWorkflowDeleteAbsenceVerificationService.h"

#include "SearchTimerQuery.h"
#include "SearchTimerResult.h"

#include <string>
#include <vector>

namespace
{

SearchTimerQuery readbackQueryFor(
    const SearchTimerDeleteResult& deleteResult)
{
    SearchTimerQuery query =
        SearchTimerQuery::byBackend(
            deleteResult.backendId);

    query.withLimit(100, 0);

    return query;
}

int sameNativeIdCount(
    const SearchTimerDeleteResult& deleteResult,
    const SearchTimerResult& readback)
{
    int count = 0;

    for (const SearchTimer& candidate : readback.items())
    {
        if (candidate.backendId() == deleteResult.backendId
            && candidate.backendNativeId() == deleteResult.backendNativeId)
        {
            ++count;
        }
    }

    return count;
}

std::string firstObservedNativeId(
    const SearchTimerDeleteResult& deleteResult,
    const SearchTimerResult& readback)
{
    for (const SearchTimer& candidate : readback.items())
    {
        if (candidate.backendId() == deleteResult.backendId
            && candidate.backendNativeId() == deleteResult.backendNativeId)
        {
            return candidate.backendNativeId();
        }
    }

    return "";
}

}

SearchTimerWorkflowBackendReadbackVerificationResult
SearchTimerWorkflowDeleteAbsenceVerificationService::verify(
    const SearchTimerDeleteResult& deleteResult,
    const ISearchTimerDataSource* dataSource) const
{
    if (!deleteResult.success)
    {
        std::vector<std::string> errors = deleteResult.errors;

        if (errors.empty())
        {
            errors.push_back(
                "delete result was not successful");
        }

        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "delete result was not successful; backend absence readback skipped",
                errors);
        result.auditTrail.push_back("deleteResultSuccess=false");
        return result;
    }

    if (deleteResult.backendId.empty())
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "delete result is missing backend id",
                {"delete result is missing backend id"});
        result.auditTrail.push_back("expectedBackendIdAvailable=false");
        return result;
    }

    if (deleteResult.backendNativeId.empty())
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "delete result is missing backend-native id",
                {"delete result is missing backend-native id"});
        result.auditTrail.push_back("expectedBackendNativeIdAvailable=false");
        return result;
    }

    if (dataSource == nullptr)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::requiredButUnavailable(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "backend readback data source unavailable",
                {"backend readback data source unavailable"});
        result.auditTrail.push_back("dataSourceAvailable=false");
        return result;
    }

    const SearchTimerResult readback =
        dataSource->list(
            readbackQueryFor(deleteResult));

    const int nativeIdCount =
        sameNativeIdCount(
            deleteResult,
            readback);

    if (nativeIdCount == 0)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::verified(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "",
                "deleted SearchTimer absence verified by backend readback");
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("sameNativeIdCount=0");
        result.auditTrail.push_back("absenceVerified=true");
        return result;
    }

    if (nativeIdCount > 1)
    {
        SearchTimerWorkflowBackendReadbackVerificationResult result =
            SearchTimerWorkflowBackendReadbackVerificationResult::ambiguousMatch(
                deleteResult.backendId,
                deleteResult.backendNativeId,
                "backend readback found multiple SearchTimers with deleted backend-native id",
                {"backend readback returned multiple deleted-id candidates"},
                {"delete absence cannot be verified while deleted backend-native id is still visible"});
        result.observedBackendNativeId =
            firstObservedNativeId(
                deleteResult,
                readback);
        result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
        result.auditTrail.push_back("sameNativeIdCount=" + std::to_string(nativeIdCount));
        result.auditTrail.push_back("absenceVerified=false");
        return result;
    }

    SearchTimerWorkflowBackendReadbackVerificationResult result =
        SearchTimerWorkflowBackendReadbackVerificationResult::failed(
            deleteResult.backendId,
            deleteResult.backendNativeId,
            firstObservedNativeId(
                deleteResult,
                readback),
            "deleted SearchTimer is still visible by backend readback",
            {"deleted SearchTimer is still visible by backend readback"});
    result.auditTrail.push_back("readbackReturnedCount=" + std::to_string(readback.returnedCount()));
    result.auditTrail.push_back("sameNativeIdCount=1");
    result.auditTrail.push_back("absenceVerified=false");
    return result;
}
