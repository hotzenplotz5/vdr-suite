#include "IHttpServer.h"

#include <cassert>
#include <cstring>
#include <memory>
#include <string>

class TestHttpServer : public IHttpServer {
public:
    HttpServerResponse handleRequest(
        const HttpServerRequest& request) const override
    {
        HttpServerResponse response;

        if (request.method == "GET" &&
            request.path == "/api/test")
        {
            response.statusCode = 200;
            response.headers["Content-Type"] = "application/json";
            response.body = "{\"ok\":true}";
            return response;
        }

        response.statusCode = 404;
        response.headers["Content-Type"] = "application/json";
        response.body = "{\"error\":\"not found\"}";

        return response;
    }
};


static void test_request_with_post_body_contract()
{
    const std::string rawRequest =
        "POST /api/vdr/timers/actions/create HTTP/1.1\r\n"
        "Host: 127.0.0.1:18080\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 45\r\n"
        "\r\n"
        "{\"backendId\":\"default\",\"timerId\":\"body-test\"}";

    assert(rawRequest.find("\r\n\r\n") != std::string::npos);
    assert(rawRequest.find("\"backendId\":\"default\"") != std::string::npos);
    assert(rawRequest.find("\"timerId\":\"body-test\"") != std::string::npos);
}

int main()
{
    test_request_with_post_body_contract();
    HttpServerRequest request;

    request.method = "GET";
    request.path = "/api/test";
    request.headers["Accept"] = "application/json";

    std::unique_ptr<IHttpServer> server =
        std::make_unique<TestHttpServer>();

    HttpServerResponse response =
        server->handleRequest(request);

    assert(response.statusCode == 200);
    assert(response.headers.at("Content-Type") == "application/json");
    assert(response.body == "{\"ok\":true}");

    HttpServerRequest missingRequest;

    missingRequest.method = "GET";
    missingRequest.path = "/api/missing";

    HttpServerResponse missingResponse =
        server->handleRequest(missingRequest);

    assert(missingResponse.statusCode == 404);
    assert(missingResponse.headers.at("Content-Type") == "application/json");
    assert(missingResponse.body == "{\"error\":\"not found\"}");

    return 0;
}
