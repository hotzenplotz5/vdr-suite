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

    assert(failed.empty());
    assert(failed.returnedCount() == 0);
    assert(failed.totalCount() == 0);
    assert(failed.limit() == 10);
    assert(failed.offset() == 5);

    std::cout << "test_restful_api_search_timer_adapter passed" << std::endl;
    return 0;
}