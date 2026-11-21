#include "MockHttpClient.h"

#include <cassert>
#include <memory>
#include <string>

int main()
{
    MockHttpClient client;

    HttpResponse response;
    response.statusCode = 202;
    response.headers["Content-Type"] = "application/json";
    response.body = "{\"ok\":true}";

    client.setResponse(response);

    HttpRequest firstRequest;
    firstRequest.method = "GET";
    firstRequest.url = "/api/status";
    firstRequest.headers["Accept"] = "application/json";

    HttpResponse firstResponse = client.execute(firstRequest);

    assert(firstResponse.statusCode == 202);
    assert(firstResponse.headers.at("Content-Type") == "application/json");
    assert(firstResponse.body == "{\"ok\":true}");

    assert(client.requestCount() == 1);
    assert(client.lastRequest().method == "GET");
    assert(client.lastRequest().url == "/api/status");
    assert(client.lastRequest().headers.at("Accept") == "application/json");

    HttpRequest secondRequest;
    secondRequest.method = "POST";
    secondRequest.url = "/api/search";
    secondRequest.body = "{\"query\":\"test\"}";

    client.execute(secondRequest);

    assert(client.requestCount() == 2);
    assert(client.requests().at(0).url == "/api/status");
    assert(client.requests().at(1).url == "/api/search");
    assert(client.lastRequest().body == "{\"query\":\"test\"}");

    std::unique_ptr<IHttpClient> interfaceClient =
        std::make_unique<MockHttpClient>();

    HttpResponse interfaceResponse = interfaceClient->execute(firstRequest);

    assert(interfaceResponse.statusCode == 200);

    return 0;
}
