#include "PublicApiRuntime.h"

#include "PublicProblemDetails.h"
#include "PublicResourcePreconditions.h"
#include "ServerBuildIdentity.h"

#include <string>
#include <utility>

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
constexpr const char* PublicOperationPrefix = "/api/v1/operations/";
constexpr const char* PublicTimerAssignmentPrefix =
    "/api/v1/timer-assignments/";

bool isPublicV1Path(const std::string& path)
{
    const std::string root(PublicApiV1Root);
    return path == root ||
        (path.size() > root.size() &&
         path.compare(0, root.size(), root) == 0 &&
         path[root.size()] == '/');
}

bool publicOperationPath(
    const std::string& path,
    std::string& operationId)
{
    const std::string prefix(PublicOperationPrefix);

    if (path.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    operationId = path.substr(prefix.size());
    return !operationId.empty() &&
        operationId.find('/') == std::string::npos;
}

bool publicTimerAssignmentPath(
    const std::string& path,
    std::string& timerAssignmentId)
{
    const std::string prefix(PublicTimerAssignmentPrefix);

    if (path.compare(0, prefix.size(), prefix) != 0)
    {
        return false;
    }

    timerAssignmentId = path.substr(prefix.size());
    return !timerAssignmentId.empty() &&
        timerAssignmentId.find('/') == std::string::npos;
}

std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (const unsigned char character : value)
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
                if (character >= 0x20U)
                {
                    escaped.push_back(
                        static_cast<char>(character));
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

void addPublicSuccessHeaders(
    ApiResponse& response,
    const std::string& requestId,
    const std::string& correlationId)
{
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Content-Type-Options"] = "nosniff";
    addRequestContextHeaders(
        response,
        requestId,
        correlationId);
}

ApiResponse jsonResponse(
    const std::string& body,
    const std::string& requestId,
    const std::string& correlationId)
{
    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    addPublicSuccessHeaders(
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
    addPublicSuccessHeaders(
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

ApiResponse invalidRequestProblem(
    const std::string& path,
    const std::string& detail,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        400,
        "invalid_request",
        "Invalid request",
        detail,
        path,
        requestId,
        correlationId);
}

ApiResponse unauthorizedProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        401,
        "unauthorized",
        "Authentication required",
        "Authentication is required for this public API resource.",
        path,
        requestId,
        correlationId);
}

ApiResponse serviceUnavailableProblem(
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        503,
        "service_unavailable",
        "Service unavailable",
        "The requested public API resource is temporarily unavailable.",
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
    const bool operationReadAvailable,
    const bool timerAssignmentReadAvailable,
    const std::string& requestId,
    const std::string& correlationId)
{
    return jsonResponse(
        "{\"apiVersion\":\"v1\",\"capabilities\":["
        "{\"id\":\"public-api.contract-root\",\"version\":1,\"availability\":\"available\"},"
        "{\"id\":\"public-api.durable-operations-read\",\"version\":1,\"availability\":\"" +
        std::string(operationReadAvailable ? "available" : "unavailable") +
        "\"},"
        "{\"id\":\"public-api.timer-assignments-read\",\"version\":1,\"availability\":\"" +
        std::string(timerAssignmentReadAvailable ? "available" : "unavailable") +
        "\"}"
        "],\"links\":{\"self\":\"/api/v1/capabilities\",\"root\":\"/api/v1\"}}",
        requestId,
        correlationId);
}

ApiResponse publicOperationResponse(
    const PublicOperationResource& operation,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            operation.resourceRevision);

    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response,
            requestId,
            correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    ApiResponse response = jsonResponse(
        "{\"operationId\":\"" + jsonEscape(operation.operationId) +
        "\",\"state\":\"" + jsonEscape(operation.state) +
        "\",\"backendId\":\"" + jsonEscape(operation.backendId) +
        "\",\"links\":{\"self\":\"" + jsonEscape(path) + "\"}}",
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

ApiResponse publicTimerAssignmentResponse(
    const PublicTimerAssignmentRevisionResource& assignment,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId,
    const std::string& ifNoneMatch)
{
    const std::string entityTag =
        vdrsuite::http::publicStrongEntityTag(
            assignment.resourceRevision);

    if (entityTag.empty())
    {
        return serviceUnavailableProblem(
            path,
            requestId,
            correlationId);
    }

    const vdrsuite::http::PublicEntityTagConditionResult condition =
        vdrsuite::http::publicEvaluateIfNoneMatch(
            ifNoneMatch,
            entityTag);

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::malformed)
    {
        return invalidRequestProblem(
            path,
            "If-None-Match is not a valid entity-tag condition.",
            requestId,
            correlationId);
    }

    if (condition ==
        vdrsuite::http::PublicEntityTagConditionResult::matched)
    {
        ApiResponse response;
        response.statusCode = 304;
        response.contentType = "application/json; charset=utf-8";
        addPublicSuccessHeaders(
            response,
            requestId,
            correlationId);
        response.headers["ETag"] = entityTag;
        return response;
    }

    const std::string self =
        path + "?backend=" + assignment.backendId;
    ApiResponse response = jsonResponse(
        "{\"timerAssignmentId\":\"" +
        jsonEscape(assignment.timerAssignmentId) +
        "\",\"backendId\":\"" +
        jsonEscape(assignment.backendId) +
        "\",\"links\":{\"self\":\"" +
        jsonEscape(self) + "\"}}",
        requestId,
        correlationId);
    response.headers["ETag"] = entityTag;
    return response;
}

}

PublicApiRuntime& PublicApiRuntime::instance()
{
    static PublicApiRuntime runtime;
    return runtime;
}

void PublicApiRuntime::registerOperationLookup(
    OperationLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    operationLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetOperationLookup()
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    operationLookup_ = OperationLookup{};
}

bool PublicApiRuntime::operationLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        operationLookupMutex_);
    return static_cast<bool>(operationLookup_);
}

void PublicApiRuntime::registerTimerAssignmentLookup(
    TimerAssignmentLookup lookup)
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = std::move(lookup);
}

void PublicApiRuntime::resetTimerAssignmentLookup()
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    timerAssignmentLookup_ = TimerAssignmentLookup{};
}

bool PublicApiRuntime::timerAssignmentLookupConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerAssignmentLookupMutex_);
    return static_cast<bool>(timerAssignmentLookup_);
}

PublicTimerAssignmentLookupResult
PublicApiRuntime::lookupTimerAssignment(
    const std::string& timerAssignmentId,
    const std::string& backendId) const
{
    TimerAssignmentLookup lookup;

    {
        std::lock_guard<std::mutex> lock(
            timerAssignmentLookupMutex_);
        lookup = timerAssignmentLookup_;
    }

    if (!lookup)
    {
        return {};
    }

    return lookup(timerAssignmentId, backendId);
}

PublicOperationLookupResult PublicApiRuntime::lookupOperation(
    const std::string& operationId,
    const std::string& actorRef) const
{
    OperationLookup lookup;

    {
        std::lock_guard<std::mutex> lock(
            operationLookupMutex_);
        lookup = operationLookup_;
    }

    if (!lookup)
    {
        return {};
    }

    return lookup(operationId, actorRef);
}

bool PublicApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    const std::string& actorRef,
    const std::string& requestId,
    const std::string& correlationId,
    ApiResponse& response,
    const std::string& ifNoneMatch,
    const std::string& authorizedBackendId) const
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
            operationLookupConfigured(),
            timerAssignmentLookupConfigured(),
            requestId,
            correlationId);
        return true;
    }

    std::string operationId;
    if (publicOperationPath(path, operationId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        const PublicOperationLookupResult found =
            lookupOperation(operationId, actorRef);

        switch (found.status)
        {
            case PublicOperationLookupStatus::ok:
                if (found.operation.operationId != operationId ||
                    found.operation.state.empty() ||
                    found.operation.backendId.empty() ||
                    found.operation.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response = publicOperationResponse(
                        found.operation,
                        path,
                        requestId,
                        correlationId,
                        ifNoneMatch);
                }
                return true;

            case PublicOperationLookupStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The operation identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicOperationLookupStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicOperationLookupStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    std::string timerAssignmentId;
    if (publicTimerAssignmentPath(
            path,
            timerAssignmentId))
    {
        if (actorRef.empty())
        {
            response = unauthorizedProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        if (authorizedBackendId.empty())
        {
            response = invalidRequestProblem(
                path,
                "An authorized backend scope is required.",
                requestId,
                correlationId);
            return true;
        }

        const PublicTimerAssignmentLookupResult found =
            lookupTimerAssignment(
                timerAssignmentId,
                authorizedBackendId);

        switch (found.status)
        {
            case PublicTimerAssignmentLookupStatus::ok:
                if (found.assignment.timerAssignmentId !=
                        timerAssignmentId ||
                    found.assignment.backendId !=
                        authorizedBackendId ||
                    found.assignment.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                }
                else
                {
                    response =
                        publicTimerAssignmentResponse(
                            found.assignment,
                            path,
                            requestId,
                            correlationId,
                            ifNoneMatch);
                }
                return true;

            case PublicTimerAssignmentLookupStatus::invalid:
                response = invalidRequestProblem(
                    path,
                    "The TimerAssignment identifier is invalid.",
                    requestId,
                    correlationId);
                return true;

            case PublicTimerAssignmentLookupStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerAssignmentLookupStatus::unavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
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
    std::string operationId;
    std::string timerAssignmentId;

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        publicOperationPath(path, operationId) ||
        publicTimerAssignmentPath(
            path,
            timerAssignmentId))
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
    std::string operationId;
    std::string timerAssignmentId;

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        publicOperationPath(path, operationId) ||
        publicTimerAssignmentPath(
            path,
            timerAssignmentId))
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
