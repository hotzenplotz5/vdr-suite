#include "PublicApiRuntime.h"

#include "ServerBuildIdentity.h"

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

bool isPublicV1Path(const std::string& path)
{
    return path == "/api/v1" ||
        path.rfind("/api/v1/", 0) == 0;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (const char character : value)
    {
        switch (character)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(character) >= 0x20)
                {
                    escaped.push_back(character);
                }
                break;
        }
    }

    return escaped;
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
    const std::string& typeSuffix,
    const std::string& code,
    const std::string& title,
    const std::string& detail,
    const std::string& instance,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/problem+json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    addRequestContextHeaders(
        response,
        requestId,
        correlationId);

    response.body =
        "{\"type\":\"urn:vdr-suite:error:" + jsonEscape(typeSuffix) +
        "\",\"title\":\"" + jsonEscape(title) +
        "\",\"status\":" + std::to_string(statusCode) +
        ",\"detail\":\"" + jsonEscape(detail) +
        "\",\"instance\":\"" + jsonEscape(instance) +
        "\",\"code\":\"" + jsonEscape(code) +
        "\",\"requestId\":\"" + jsonEscape(requestId) + "\"";

    if (!correlationId.empty())
    {
        response.body +=
            ",\"correlationId\":\"" +
            jsonEscape(correlationId) + "\"";
    }

    response.body += "}";
    return response;
}

ApiResponse notFoundProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        404,
        "not-found",
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
        "method-not-allowed",
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

    if (!isPublicV1Path(path))
    {
        return false;
    }

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities")
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId);
        return true;
    }

    response = notFoundProblem(
        path,
        requestId,
        correlationId);
    return true;
}
