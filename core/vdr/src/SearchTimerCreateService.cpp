#include "SearchTimerCreateService.h"

SearchTimerCreateResult SearchTimerCreateService::create(
    const SearchTimerCreateRequest& request,
    ISearchTimerCommandExecutor& executor) const
{
    if (!request.hasBackendId())
    {
        return SearchTimerCreateResult::failed(
            "searchtimer backend id is required",
            {"backendId is required"});
    }

    if (!request.hasName())
    {
        return SearchTimerCreateResult::failed(
            "searchtimer name is required",
            {"name is required"});
    }

    if (!request.hasQuery())
    {
        return SearchTimerCreateResult::failed(
            "searchtimer query is required",
            {"query is required"});
    }

    return executor.create(request);
}
