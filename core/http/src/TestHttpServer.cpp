#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "DashboardController.h"

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter)
{
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (request.method != "GET")
    {
        return mapApiResponse(
            405,
            "application/json",
            "{\"error\":\"method not allowed\"}");
    }

    ApiResponse apiResponse =
        apiRouter_.handleGet(request.path);

    return mapApiResponse(
        apiResponse.statusCode,
        apiResponse.contentType,
        apiResponse.body);
}

HttpServerResponse TestHttpServer::mapApiResponse(
    int statusCode,
    const std::string& contentType,
    const std::string& body) const
{
    HttpServerResponse response;

    response.statusCode = statusCode;
    response.headers["Content-Type"] = contentType;
    response.body = body;

    return response;
}
