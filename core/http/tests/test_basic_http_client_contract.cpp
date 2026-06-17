#include "BasicHttpClient.h"

#include <cassert>
#include <string>

int main()
{
    BasicHttpClientConfig config;
    config.host = "127.0.0.1";
    config.port = 8002;
    config.timeoutSeconds = 3;

    BasicHttpClient client(config);

    assert(client.config().host == "127.0.0.1");
    assert(client.config().port == 8002);
    assert(client.config().timeoutSeconds == 3);

    HttpRequest request;
    request.method = "GET";
    request.url = "/api/info.json";

    const HttpResponse response =
        client.execute(request);

    assert(response.statusCode == 0);
    assert(response.body.find("BasicHttpClient network execution is not implemented yet")
           != std::string::npos);
    assert(response.body.find("GET /api/info.json") != std::string::npos);

    return 0;
}
