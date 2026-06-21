#include "SearchTimerDeleteService.h"

SearchTimerDeleteResult SearchTimerDeleteService::remove(
    const SearchTimerDeleteRequest& request,
    ISearchTimerCommandExecutor& executor) const
{
    if (!request.hasBackendId())
    {
        return SearchTimerDeleteResult::failed(
            "searchtimer backend id is required",
            {"backendId is required"});
    }

    if (!request.hasBackendNativeId())
    {
        return SearchTimerDeleteResult::failed(
            "searchtimer backend native id is required",
            {"backendNativeId is required"});
    }

    return executor.remove(request);
}
