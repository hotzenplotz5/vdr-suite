#include "RestfulApiSearchTimerCommandExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"
#include "SearchTimerCreateRequest.h"
#include "SearchTimerDeleteRequest.h"
#include "SearchTimerUpdateRequest.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

class TestHttpClient final : public IHttpClient
{
public:
    explicit TestHttpClient(
        HttpResponse response)
        : response_(response)
    {
    }

    HttpResponse execute(
        const HttpRequest& request) const override
    {
        requests.push_back(request);
        return response_;
    }

    mutable std::vector<HttpRequest> requests;

private:
    HttpResponse response_;
};

static SearchTimerCreateRequest makeCreateRequest()
{
    SearchTimerCreateRequest request;
    request.backendId = "home-vdr";
    request.name = "Tatort";
    request.query = "Tatort";
    request.active = true;
    return request;
}

static void test_create_posts_to_restfulapi_searchtimers()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK, Id:42";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerCreateResult result =
        executor.create(makeCreateRequest());

    assert(result.success == true);
    assert(result.searchTimer.id().backendId() == "home-vdr");
    assert(result.searchTimer.id().nativeId() == "42");
    assert(result.searchTimer.query() == "Tatort");
    assert(httpClient.requests.size() == 1);
    assert(httpClient.requests.at(0).method == "POST");
    assert(httpClient.requests.at(0).url.find("/searchtimers?") == 0);
    assert(httpClient.requests.at(0).url.find("search=Tatort") != std::string::npos);
    assert(httpClient.requests.at(0).url.find("use_as_searchtimer=1") != std::string::npos);
}

static void test_create_fails_without_created_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "OK";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const SearchTimerCreateResult result =
        executor.create(makeCreateRequest());

    assert(result.success == false);
    assert(result.message == "RESTfulAPI searchtimer create did not return an id");
    assert(result.errors.size() == 1);
}

static void test_update_returns_unsupported_without_http_call()
{
    HttpResponse response;
    response.statusCode = 200;

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    SearchTimerUpdateRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "42";
    request.name = "Tatort";
    request.query = "Tatort";
    request.active = true;

    const SearchTimerUpdateResult result =
        executor.update(request);

    assert(result.success == false);
    assert(result.message == "RESTfulAPI searchtimer update is not supported");
    assert(result.errors.size() == 1);
    assert(httpClient.requests.empty());
}

static void test_remove_deletes_restfulapi_searchtimer_by_native_id()
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = "Searchtimer deleted.";

    TestHttpClient httpClient(response);
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    SearchTimerDeleteRequest request;
    request.backendId = "home-vdr";
    request.backendNativeId = "42";

    const SearchTimerDeleteResult result =
        executor.remove(request);

    assert(result.success == true);
    assert(result.backendId == "home-vdr");
    assert(result.backendNativeId == "42");
    assert(httpClient.requests.size() == 1);
    assert(httpClient.requests.at(0).method == "DELETE");
    assert(httpClient.requests.at(0).url == "/searchtimers/42");
}

int main()
{
    test_create_posts_to_restfulapi_searchtimers();
    test_create_fails_without_created_id();
    test_update_returns_unsupported_without_http_call();
    test_remove_deletes_restfulapi_searchtimer_by_native_id();

    std::cout
        << "test_restfulapi_search_timer_command_executor passed"
        << std::endl;

    return 0;
}
