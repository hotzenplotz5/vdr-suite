#include "RestfulApiSearchTimerAdapter.h"

#include "MockHttpClient.h"

#include <cassert>
#include <iostream>

int main()
{
    MockHttpClient httpClient;
    HttpResponse response;
    response.statusCode = 200;
    response.body =
        "{\"searchtimers\":["
        "{\"id\":1,\"search\":\"Terra X\",\"use_as_searchtimer\":1},"
        "{\"id\":2,\"search\":\"Tatort\",\"use_as_searchtimer\":0},"
        "{\"id\":3,\"search\":\"Tagesschau\",\"use_as_searchtimer\":1}"
        "],\"count\":3,\"total\":3}";
    httpClient.setResponse(response);

    RestfulApiSearchTimerAdapter adapter(
        "livingroom",
        httpClient);

    SearchTimerResult all = adapter.list(
        SearchTimerQuery::all());

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/searchtimers.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");
    assert(all.available());
    assert(all.returnedCount() == 3);
    assert(all.totalCount() == 3);
    assert(all.items().at(0).backendId() == "livingroom");
    assert(all.items().at(0).backendNativeId() == "1");

    SearchTimerResult active = adapter.list(
        SearchTimerQuery::byState(SearchTimerState::Active));

    assert(active.returnedCount() == 2);
    assert(active.totalCount() == 2);
    assert(active.items().at(0).name() == "Terra X");
    assert(active.items().at(1).name() == "Tagesschau");

    SearchTimerResult text = adapter.list(
        SearchTimerQuery::byText("Tatort"));

    assert(text.returnedCount() == 1);
    assert(text.totalCount() == 1);
    assert(text.items().at(0).isInactive());

    SearchTimerResult page = adapter.list(
        SearchTimerQuery::limited(1, 1));

    assert(page.returnedCount() == 1);
    assert(page.totalCount() == 3);
    assert(page.limit() == 1);
    assert(page.offset() == 1);
    assert(page.items().at(0).name() == "Tatort");

    HttpResponse failingResponse;
    failingResponse.statusCode = 403;
    failingResponse.body = "{\"error\":\"Epgsearch isn't installed!\"}";
    httpClient.setResponse(failingResponse);

    SearchTimerResult failed = adapter.list(
        SearchTimerQuery::limited(10, 5));

    assert(!failed.available());
    assert(failed.empty());
    assert(failed.returnedCount() == 0);
    assert(failed.totalCount() == 0);
    assert(failed.limit() == 10);
    assert(failed.offset() == 5);

    HttpResponse emptyResponse;
    emptyResponse.statusCode = 200;
    emptyResponse.body = "{\"searchtimers\":[],\"count\":0,\"total\":0}";
    httpClient.setResponse(emptyResponse);
    SearchTimerResult genuineEmpty = adapter.list(
        SearchTimerQuery::limited(10, 0));
    assert(genuineEmpty.available());
    assert(genuineEmpty.empty());
    assert(genuineEmpty.totalCount() == 0);

    HttpResponse malformedResponse;
    malformedResponse.statusCode = 200;
    malformedResponse.body = "{\"count\":0}";
    httpClient.setResponse(malformedResponse);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    // A 200 with a truncated catalogue must NOT be treated as an empty
    // authoritative snapshot; that could retire live public identities.
    HttpResponse incomplete;
    incomplete.statusCode = 200;
    incomplete.body = "{\"searchtimers\":[],\"count\":0,\"total\":1}";
    httpClient.setResponse(incomplete);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    incomplete.body = "{\"searchtimers\":[{\"id\":-1,"
        "\"search\":\"invalid\"}],\"count\":1,\"total\":1}";
    httpClient.setResponse(incomplete);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    incomplete.body = "{\"searchtimers\":[],\"count\":1,\"total\":1}";
    httpClient.setResponse(incomplete);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    incomplete.body = "{\"searchtimers\":[null],\"count\":0,\"total\":0}";
    httpClient.setResponse(incomplete);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    incomplete.body = "{\"searchtimers\":[{\"id\":1,"
        "\"search\":\"valid\"},],\"count\":1,\"total\":1}";
    httpClient.setResponse(incomplete);
    assert(!adapter.list(SearchTimerQuery::all()).available());

    HttpResponse unavailableResponse;
    unavailableResponse.statusCode = 503;
    httpClient.setResponse(unavailableResponse);
    SearchTimerResult unavailable = adapter.list(SearchTimerQuery::all());
    assert(!unavailable.available());
    assert(unavailable.empty());

    std::cout << "test_restful_api_search_timer_adapter passed" << std::endl;
    return 0;
}