RestfulApiSearchTimerCommandExecutor::RestfulApiSearchTimerCommandExecutor(
    IHttpClient& httpClient)
    : httpClient_(httpClient)
{
}

SearchTimerCreateResult RestfulApiSearchTimerCommandExecutor::create(
    const SearchTimerCreateRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "POST";
    httpRequest.url = "/searchtimers";
    httpRequest.headers["Accept"] = "text/plain";
    httpRequest.headers["Content-Type"] = "application/json";
    httpRequest.body = buildSearchTimerBody(request, "");

    const HttpResponse response = httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerCreateResult::failed(
            "RESTfulAPI searchtimer create failed",
            {
                "status=" + std::to_string(response.statusCode),
                "requestBody=" + httpRequest.body,
                "responseBody=" + response.body
            });
    }

    // No query-only post-CREATE lookup: the durable dispatcher already has
    // a complete pre/post provider inventory and must resolve native ID
    // there. HTTP 200 without Id is never sufficient to report success
    // from the Public-v1 operation.
    const std::string createdId = extractReturnedId(response.body);

    return SearchTimerCreateResult::ok(
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(request.backendId, createdId),
            request.name,
            request.query,
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive),
        createdId.empty()
            ? "RESTfulAPI accepted CREATE; native ID must be reconciled"
            : "searchtimer created through RESTfulAPI");
}

SearchTimerUpdateResult RestfulApiSearchTimerCommandExecutor::update(
    const SearchTimerUpdateRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "POST";
    httpRequest.url = "/searchtimers";
    httpRequest.headers["Accept"] = "text/plain";
    httpRequest.headers["Content-Type"] = "application/json";
    httpRequest.body = buildSearchTimerBody(request, request.backendNativeId);

    const HttpResponse response = httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerUpdateResult::failed(
            "RESTfulAPI searchtimer update failed",
            {
                "status=" + std::to_string(response.statusCode),
                "requestBody=" + httpRequest.body,
                "responseBody=" + response.body
            });
    }

    const std::string returnedId = extractReturnedId(response.body);
    const std::string effectiveId =
        returnedId.empty() ? request.backendNativeId : returnedId;

    return SearchTimerUpdateResult::ok(
        SearchTimer::create(
            SearchTimerId::fromBackendNativeId(request.backendId, effectiveId),
            request.name,
            request.query,
            request.active
                ? SearchTimerState::Active
                : SearchTimerState::Inactive),
        "searchtimer updated through RESTfulAPI");
}

SearchTimerDeleteResult RestfulApiSearchTimerCommandExecutor::remove(
    const SearchTimerDeleteRequest& request)
{
    HttpRequest httpRequest;
    httpRequest.method = "DELETE";
    httpRequest.url = "/searchtimers/" + urlEncode(request.backendNativeId);
    httpRequest.headers["Accept"] = "text/plain";

    const HttpResponse response = httpClient_.execute(httpRequest);

    if (response.statusCode != 200)
    {
        return SearchTimerDeleteResult::failed(
            "RESTfulAPI searchtimer delete failed",
            {response.body});
    }

    return SearchTimerDeleteResult::ok(
        request.backendId,
        request.backendNativeId,
        "searchtimer deleted through RESTfulAPI");
}
