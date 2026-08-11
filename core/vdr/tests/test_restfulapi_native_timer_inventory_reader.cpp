#include "IHttpClient.h"
#include "RestfulApiNativeTimerInventoryReader.h"

#include <cassert>
#include <iostream>
#include <string>

using namespace vdrsuite::vdr;
using namespace vdrsuite::timers;

namespace
{
class FakeHttpClient final : public IHttpClient
{
public:
    HttpResponse response;
    mutable int calls = 0;
    mutable HttpRequest lastRequest;

    HttpResponse execute(const HttpRequest& request) const override
    {
        ++calls;
        lastRequest = request;
        return response;
    }
};

RestfulApiNativeTimerInventoryReadRequest request()
{
    RestfulApiNativeTimerInventoryReadRequest value;
    value.backendId = "backend:a";
    value.backendGeneration = 8;
    value.observedAt = 2200;
    return value;
}
}

int main()
{
    FakeHttpClient http;
    RestfulApiNativeTimerInventoryReader reader(http);

    http.response.statusCode = 200;
    http.response.body = R"({"timers":[]})";
    auto empty = reader.read(request());
    assert(empty.ok());
    assert(nativeTimerInventoryEvidenceValid(empty.evidence));
    assert(empty.evidence.backendNativeTimerIds.empty());
    assert(http.lastRequest.method == "GET");
    assert(http.lastRequest.url == "/timers.json");
    assert(http.lastRequest.headers.at("Accept") == "application/json");

    http.response.body = R"({"timers":[{"id":"17","title":"A"},{"number":4,"title":"B"}],"count":2})";
    auto complete = reader.read(request());
    assert(complete.ok());
    assert(complete.evidence.backendNativeTimerIds.size() == 2);
    assert(complete.evidence.backendNativeTimerIds[0] == "17");
    assert(complete.evidence.backendNativeTimerIds[1] == "4");

    http.response.statusCode = 503;
    http.response.body = R"({"timers":[]})";
    auto httpError = reader.read(request());
    assert(httpError.status == RestfulApiNativeTimerInventoryReadStatus::httpError);
    assert(httpError.httpStatusCode == 503);
    assert(httpError.evidence.completeness == NativeTimerInventoryCompleteness::unknown);

    http.response.statusCode = 200;
    http.response.body = R"({"timers":)";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"count":0})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"timers":[{"title":"missing id"}]})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"timers":[{"id":"17"},{"id":"17"}]})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"([{"id":"20"},{"number":"3"}])";
    auto rootArray = reader.read(request());
    assert(rootArray.ok());
    assert(rootArray.evidence.backendNativeTimerIds[0] == "20");
    assert(rootArray.evidence.backendNativeTimerIds[1] == "3");

    http.response.body = R"({"meta":{"nested":[true,false,null,{"x":"y"}]},"timers":[{"id":"21"}]})";
    assert(reader.read(request()).ok());

    http.response.body = R"({"timers":[{"id":"21","id":"22"}]})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"timers":[{"number":-1}]})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"timers":[{"number":4.5}]})";
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = std::string(4 * 1024 * 1024 + 1, 'x');
    assert(reader.read(request()).status == RestfulApiNativeTimerInventoryReadStatus::parseError);

    http.response.body = R"({"timers":[{"id":"" ,"number":7}]})";
    auto fallbackNumber = reader.read(request());
    assert(fallbackNumber.ok());
    assert(fallbackNumber.evidence.backendNativeTimerIds[0] == "7");

    const int beforeInvalid = http.calls;
    auto invalid = request();
    invalid.backendGeneration = 0;
    assert(reader.read(invalid).status == RestfulApiNativeTimerInventoryReadStatus::invalidRequest);
    assert(http.calls == beforeInvalid);

    std::cout << "test_restfulapi_native_timer_inventory_reader passed\n";
    return 0;
}
