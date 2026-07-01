#include "TestHttpServer.h"

#include "ApiRouter.h"
#include "DashboardController.h"

#include <cstdlib>
#include <cctype>
#include <string>

namespace {

std::string toLowerAscii(const std::string& value)
{
    std::string lowered;
    lowered.reserve(value.size());

    for (const char character : value)
    {
        lowered.push_back(
            static_cast<char>(
                std::tolower(
                    static_cast<unsigned char>(character))));
    }

    return lowered;
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& headerName)
{
    const std::string wanted =
        toLowerAscii(headerName);

    for (const auto& header : request.headers)
    {
        if (toLowerAscii(header.first) == wanted)
        {
            return header.second;
        }
    }

    return "";
}

std::string expectedAuthorizationHeader()
{
    const char* configured =
        std::getenv("VDR_SUITE_BASIC_AUTH");

    if (configured != nullptr && configured[0] != '\0')
    {
        return configured;
    }

    return "Basic YWRtaW46dmRyLXN1aXRl";
}

bool isAuthorized(const HttpServerRequest& request)
{
    return headerValue(request, "Authorization") ==
        expectedAuthorizationHeader();
}

HttpServerResponse makeUnauthorizedResponse()
{
    HttpServerResponse response;

    response.statusCode = 401;
    response.headers["Content-Type"] = "application/json";
    response.headers["WWW-Authenticate"] =
        "Basic realm=\"VDR-Suite\", charset=\"UTF-8\"";
    response.body = "{\"error\":\"authentication required\"}";

    return response;
}

}

TestHttpServer::TestHttpServer(ApiRouter& apiRouter)
    : apiRouter_(apiRouter)
{
}

HttpServerResponse TestHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!isAuthorized(request))
    {
        return makeUnauthorizedResponse();
    }

    ApiResponse apiResponse;

    if (request.method == "GET")
    {
        apiResponse =
            apiRouter_.handleGet(request.path);
    }
    else if (request.method == "POST")
    {
        apiResponse =
            apiRouter_.handlePost(
                request.path,
                request.body);
    }
    else
    {
        return mapApiResponse(
            405,
            "application/json",
            "{\"error\":\"method not allowed\"}");
    }

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
