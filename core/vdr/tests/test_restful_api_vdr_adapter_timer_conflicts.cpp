#include "MockHttpClient.h"
#include "RestfulApiVdrAdapter.h"

#include <cassert>
#include <string>

int main()
{
    VdrConfig config;
    config.enabled = true;
    config.mode = "restfulapi";
    config.host = "127.0.0.1";
    config.port = 8002;

    MockHttpClient httpClient;

    HttpResponse response;
    response.statusCode = 200;
    response.body =
        R"JSON({"check_advised":false,"conflicts":["1783260840:14|84|11#12#14#13"],"count":1,"total":1})JSON";
    httpClient.setResponse(response);

    RestfulApiVdrAdapter adapter(config, httpClient);

    const VdrTimerConflictReport report =
        adapter.getTimerConflictReport();

    assert(httpClient.requestCount() == 1);
    assert(httpClient.lastRequest().method == "GET");
    assert(httpClient.lastRequest().url == "/searchtimers/conflicts.json");
    assert(httpClient.lastRequest().headers.at("Accept") == "application/json");

    assert(report.available);
    assert(report.count == 1);
    assert(report.total == 1);
    assert(report.conflicts.size() == 1);
    assert(report.conflicts[0].conflictTime == 1783260840);
    assert(report.conflicts[0].entries[0].timerIndex == 14);

    return 0;
}
