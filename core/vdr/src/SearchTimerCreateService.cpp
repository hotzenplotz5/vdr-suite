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

    // The legacy browser controller owns one RESTfulAPI client. Never
    // dispatch a selected second backend through the first provider.
    if (!providerBackendId_.empty() && request.backendId != providerBackendId_)
    {
        return SearchTimerCreateResult::failed(
            "searchtimer backend does not match provider",
            {"selected backend is not served by this SearchTimer endpoint"});
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
