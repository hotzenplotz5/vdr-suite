#include "SearchTimerUpdateService.h"

SearchTimerUpdateResult SearchTimerUpdateService::update(
    const SearchTimerUpdateRequest& request,
    ISearchTimerCommandExecutor& executor) const
{
    if (!request.hasBackendId())
    {
        return SearchTimerUpdateResult::failed(
            "searchtimer backend id is required",
            {"backendId is required"});
    }

    if (!request.hasBackendNativeId())
    {
        return SearchTimerUpdateResult::failed(
            "searchtimer backend native id is required",
            {"backendNativeId is required"});
    }

    if (!request.hasName())
    {
        return SearchTimerUpdateResult::failed(
            "searchtimer name is required",
            {"name is required"});
    }

    if (!request.hasQuery())
    {
        return SearchTimerUpdateResult::failed(
            "searchtimer query is required",
            {"query is required"});
    }

    return executor.update(request);
}
