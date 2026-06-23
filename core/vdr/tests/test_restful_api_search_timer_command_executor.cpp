#include "RestfulApiSearchTimerCommandExecutor.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"
#include "SearchTimerCreateRequest.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace
{
class SequencedHttpClient final : public IHttpClient
{
public:
    explicit SequencedHttpClient(
        std::vector<HttpResponse> responses)
        : responses_(std::move(responses))
    {
    }

    HttpResponse execute(
        const HttpRequest& request) const override
    {
        requests_.push_back(request);

        assert(nextResponse_ < responses_.size());

        return responses_[nextResponse_++];
    }

    const std::vector<HttpRequest>& requests() const
    {
        return requests_;
    }

private:
    std::vector<HttpResponse> responses_;
    mutable std::vector<HttpRequest> requests_;
    mutable std::size_t nextResponse_ = 0;
};

SearchTimerCreateRequest makeCreateRequest(
    const std::string& query)
{
    SearchTimerCreateRequest request;
    request.backendId = "default";
    request.name = query;
    request.query = query;
    request.active = true;
    request.directory = "VDR-Suite-Validation";
    request.compareTitle = true;
    request.compareSubtitle = true;
    request.compareSummary = true;
    request.matchMode = 5;
    request.matchTolerance = 2;
    return request;
}

void createUsesReturnedTextIdWhenAvailable()
{
    HttpResponse createResponse;
    createResponse.statusCode = 200;
    createResponse.body = "Id: 17";

    SequencedHttpClient httpClient({createResponse});
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const auto result =
        executor.create(
            makeCreateRequest("VDR-Suite Native Fuzzy Direct Id"));

    assert(result.success);
    assert(result.searchTimer.backendId() == "default");
    assert(result.searchTimer.backendNativeId() == "17");
    assert(result.searchTimer.query() == "VDR-Suite Native Fuzzy Direct Id");
    assert(httpClient.requests().size() == 1);
    assert(httpClient.requests()[0].method == "POST");
    assert(httpClient.requests()[0].url == "/searchtimers");
}

void createFallsBackToReadbackWhenCreateBodyHasNoId()
{
    const std::string query =
        "VDR-Suite Native Fuzzy Empty Body";

    HttpResponse createResponse;
    createResponse.statusCode = 200;
    createResponse.body = "";

    HttpResponse readbackResponse;
    readbackResponse.statusCode = 200;
    readbackResponse.body =
        "{"
        "\"searchtimers\":["
        "{"
        "\"id\":41,"
        "\"search\":\"Other SearchTimer\","
        "\"mode\":5,"
        "\"tolerance\":2"
        "},"
        "{"
        "\"id\":42,"
        "\"search\":\"VDR-Suite Native Fuzzy Empty Body\","
        "\"mode\":5,"
        "\"tolerance\":2"
        "}"
        "]"
        "}";

    SequencedHttpClient httpClient({
        createResponse,
        readbackResponse
    });
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const auto result =
        executor.create(
            makeCreateRequest(query));

    assert(result.success);
    assert(result.searchTimer.backendId() == "default");
    assert(result.searchTimer.backendNativeId() == "42");
    assert(result.searchTimer.query() == query);

    assert(httpClient.requests().size() == 2);
    assert(httpClient.requests()[0].method == "POST");
    assert(httpClient.requests()[0].url == "/searchtimers");
    assert(httpClient.requests()[1].method == "GET");
    assert(httpClient.requests()[1].url == "/searchtimers.json");
}

void createFailsWhenReadbackCannotFindUniqueCreatedId()
{
    const std::string query =
        "VDR-Suite Native Fuzzy Ambiguous";

    HttpResponse createResponse;
    createResponse.statusCode = 200;
    createResponse.body = "";

    HttpResponse readbackResponse;
    readbackResponse.statusCode = 200;
    readbackResponse.body =
        "{"
        "\"searchtimers\":["
        "{"
        "\"id\":1,"
        "\"search\":\"VDR-Suite Native Fuzzy Ambiguous\""
        "},"
        "{"
        "\"id\":2,"
        "\"search\":\"VDR-Suite Native Fuzzy Ambiguous\""
        "}"
        "]"
        "}";

    SequencedHttpClient httpClient({
        createResponse,
        readbackResponse
    });
    RestfulApiSearchTimerCommandExecutor executor(httpClient);

    const auto result =
        executor.create(
            makeCreateRequest(query));

    assert(!result.success);
    assert(result.message == "RESTfulAPI searchtimer create did not return an id");
    assert(!result.errors.empty());
    assert(httpClient.requests().size() == 2);
}
}

int main()
{
    createUsesReturnedTextIdWhenAvailable();
    createFallsBackToReadbackWhenCreateBodyHasNoId();
    createFailsWhenReadbackCannotFindUniqueCreatedId();

    std::cout
        << "test_restful_api_search_timer_command_executor passed"
        << std::endl;

    return 0;
}
