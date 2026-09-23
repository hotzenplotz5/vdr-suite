#include "PublicApiRuntime.h"

#include "ServerBuildIdentity.h"
#include "PublicProblemDetails.h"

#include <string>

namespace
{

std::string requestPath(const std::string& requestTarget)
{
    const std::size_t separator = requestTarget.find('?');
    return separator == std::string::npos
        ? requestTarget
        : requestTarget.substr(0, separator);
}

constexpr const char* PublicApiV1Root = "/api/v1";

bool isPublicV1Path(const std::string& path)
{
    const std::string root(PublicApiV1Root);
    return path == root ||
        (path.size() > root.size() &&
         path.compare(0, root.size(), root) == 0 &&
         path[root.size()] == '/');
}

void addRequestContextHeaders(
    ApiResponse& response,
    const std::string& requestId,
    const std::string& correlationId)
{
    if (!requestId.empty())
    {
        response.headers["X-Request-ID"] = requestId;
    }

    if (!correlationId.empty())
    {
        response.headers["X-Correlation-ID"] = correlationId;
    }
}

ApiResponse jsonResponse(
    const std::string& body,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    addRequestContextHeaders(
        response,
        requestId,
        correlationId);
    response.body = body;
    return response;
}

ApiResponse problemResponse(
    int statusCode,
    const std::string& code,
    const std::string& title,
    const std::string& detail,
    const std::string& instance,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = PublicProblemDetails::contentType();
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    addRequestContextHeaders(
        response,
        requestId,
        correlationId);

    PublicProblemDetails problem;
    problem.statusCode = statusCode;
    problem.code = code;
    problem.title = title;
    problem.detail = detail;
    problem.instance = instance;
    problem.requestId = requestId;
    problem.correlationId = correlationId;
    response.body = problem.serialize();
    return response;
}

ApiResponse notFoundProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        404,
        "not_found",
        "Resource not found",
        "The requested public API resource is not available.",
        path,
        requestId,
        correlationId);
}

ApiResponse methodNotAllowedProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response = problemResponse(
        405,
        "method_not_allowed",
        "Method not allowed",
        "The requested public API resource does not support this method.",
        path,
        requestId,
        correlationId);
    response.headers["Allow"] = "GET";
    return response;
}

ApiResponse contractRoot(
    const bool authenticated,
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        std::string("{\"apiVersion\":\"v1\",\"serverVersion\":\"")
        + VdrSuiteServerBuildIdentity::ServerVersion
        + "\",\"supportedApiMajors\":[\"v1\"],\"authentication\":{\"authenticated\":"
        + (authenticated ? "true" : "false")
        + "},\"links\":{\"self\":\"/api/v1\",\"capabilities\":\"/api/v1/capabilities\"}}",
        requestId,
        correlationId);
}

ApiResponse platformCapabilities(
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        "{\"apiVersion\":\"v1\",\"capabilities\":["
        "{\"id\":\"public-api.contract-root\",\"version\":1,\"availability\":\"available\"}"
        "],\"links\":{\"self\":\"/api/v1/capabilities\",\"root\":\"/api/v1\"}}",
        requestId,
        correlationId);
}

}

PublicApiRuntime& PublicApiRuntime::instance()
{
    static PublicApiRuntime runtime;
    return runtime;
}

bool PublicApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    const std::string& actorRef,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);

    if (path == "/api/v1")
    {
        response = contractRoot(
            !actorRef.empty(),
            requestId,
            correlationId);
        return true;
    }

    if (path == "/api/v1/capabilities")
    {
        response = platformCapabilities(
            requestId,
            correlationId);
        return true;
    }

    if (isPublicV1Path(path))
    {
        response = notFoundProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}

bool PublicApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities")
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}

bool PublicApiRuntime::tryHandleUnsupportedMethod(
    const std::string& method,
    const std::string& requestTarget,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response) const
{
    if (method == "GET" ||
        method == "POST")
    {
        return false;
    }

    const std::string path = requestPath(requestTarget);

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities")
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    if (isPublicV1Path(path))
    {
        response = notFoundProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    return false;
}
