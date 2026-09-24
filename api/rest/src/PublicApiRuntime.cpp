#include "PublicApiRuntime.h"

#include "PublicProblemDetails.h"
#include "PublicResourcePreconditions.h"
#include "ServerBuildIdentity.h"

#include <cctype>
#include <cstddef>
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

std::string lowerAscii(std::string value)
{
    for (char& character : value)
    {
        character = static_cast<char>(
            std::tolower(static_cast<unsigned char>(character)));
    }
    return value;
}

bool asciiWhitespace(char character)
{
    return character == ' ' || character == '\t' ||
        character == '\r' || character == '\n';
}

std::string trimAsciiWhitespace(std::string value)
{
    std::size_t begin = 0;
    while (begin < value.size() && asciiWhitespace(value[begin]))
        ++begin;

    std::size_t end = value.size();
    while (end > begin && asciiWhitespace(value[end - 1U]))
        --end;

    return value.substr(begin, end - begin);
}

bool applicationJsonContentType(const std::string& value)
{
    if (value.empty()) return false;
    const std::size_t separator = value.find(';');
    const std::string mediaType = trimAsciiWhitespace(
        value.substr(0, separator));
    return lowerAscii(mediaType) == "application/json";
}

bool publicIdempotencyKeyValid(const std::string& value)
{
    if (value.empty() || value.size() > 160U) return false;
    for (const unsigned char character : value)
    {
        if (character < 0x21U || character > 0x7eU)
            return false;
    }
    return true;
}

class JsonSyntaxValidator
{
public:
    explicit JsonSyntaxValidator(const std::string& input)
        : input_(input)
    {
    }

    bool valid()
    {
        position_ = 0;
        skipWhitespace();
        if (!parseValue(0U)) return false;
        skipWhitespace();
        return position_ == input_.size();
    }

private:
    void skipWhitespace()
    {
        while (position_ < input_.size() &&
               asciiWhitespace(input_[position_]))
        {
            ++position_;
        }
    }

    bool parseValue(std::size_t depth)
    {
        if (depth > 16U || position_ >= input_.size())
            return false;

        const char current = input_[position_];
        if (current == '"') return parseString();
        if (current == '{') return parseObject(depth + 1U);
        if (current == '[') return parseArray(depth + 1U);
        if (current == 't') return parseLiteral("true");
        if (current == 'f') return parseLiteral("false");
        if (current == 'n') return parseLiteral("null");
        return parseNumber();
    }

    bool parseLiteral(const char* literal)
    {
        const std::string value(literal);
        if (input_.compare(position_, value.size(), value) != 0)
            return false;
        position_ += value.size();
        return true;
    }

    bool parseString()
    {
        if (position_ >= input_.size() ||
            input_[position_] != '"')
        {
            return false;
        }
        ++position_;

        while (position_ < input_.size())
        {
            const unsigned char character =
                static_cast<unsigned char>(input_[position_++]);

            if (character == '"') return true;
            if (character < 0x20U) return false;
            if (character != '\\') continue;

            if (position_ >= input_.size()) return false;
            const char escaped = input_[position_++];
            if (escaped == '"' || escaped == '\\' ||
                escaped == '/' || escaped == 'b' ||
                escaped == 'f' || escaped == 'n' ||
                escaped == 'r' || escaped == 't')
            {
                continue;
            }

            if (escaped != 'u' ||
                position_ + 4U > input_.size())
            {
                return false;
            }

            for (std::size_t index = 0; index < 4U; ++index)
            {
                const unsigned char hex =
                    static_cast<unsigned char>(
                        input_[position_ + index]);
                if (!std::isxdigit(hex)) return false;
            }
            position_ += 4U;
        }

        return false;
    }

    bool parseNumber()
    {
        const std::size_t start = position_;
        if (position_ < input_.size() &&
            input_[position_] == '-')
        {
            ++position_;
        }
        if (position_ >= input_.size()) return false;

        if (input_[position_] == '0')
        {
            ++position_;
        }
        else
        {
            if (input_[position_] < '1' ||
                input_[position_] > '9')
            {
                return false;
            }
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
        }

        if (position_ < input_.size() &&
            input_[position_] == '.')
        {
            ++position_;
            const std::size_t fractionStart = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == fractionStart) return false;
        }

        if (position_ < input_.size() &&
            (input_[position_] == 'e' ||
             input_[position_] == 'E'))
        {
            ++position_;
            if (position_ < input_.size() &&
                (input_[position_] == '+' ||
                 input_[position_] == '-'))
            {
                ++position_;
            }
            const std::size_t exponentStart = position_;
            while (position_ < input_.size() &&
                   input_[position_] >= '0' &&
                   input_[position_] <= '9')
            {
                ++position_;
            }
            if (position_ == exponentStart) return false;
        }

        return position_ > start;
    }

    bool parseObject(std::size_t depth)
    {
        ++position_;
        skipWhitespace();
        if (position_ < input_.size() &&
            input_[position_] == '}')
        {
            ++position_;
            return true;
        }

        while (position_ < input_.size())
        {
            if (!parseString()) return false;
            skipWhitespace();
            if (position_ >= input_.size() ||
                input_[position_] != ':')
            {
                return false;
            }
            ++position_;
            skipWhitespace();
            if (!parseValue(depth)) return false;
            skipWhitespace();

            if (position_ >= input_.size()) return false;
            if (input_[position_] == '}')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            skipWhitespace();
        }
        return false;
    }

    bool parseArray(std::size_t depth)
    {
        ++position_;
        skipWhitespace();
        if (position_ < input_.size() &&
            input_[position_] == ']')
        {
            ++position_;
            return true;
        }

        while (position_ < input_.size())
        {
            if (!parseValue(depth)) return false;
            skipWhitespace();
            if (position_ >= input_.size()) return false;
            if (input_[position_] == ']')
            {
                ++position_;
                return true;
            }
            if (input_[position_] != ',') return false;
            ++position_;
            skipWhitespace();
        }
        return false;
    }

    const std::string& input_;
    std::size_t position_ = 0;
};

bool emptyJsonObject(const std::string& input)
{
    std::size_t position = 0;
    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    if (position >= input.size() || input[position] != '{')
        return false;
    ++position;

    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    if (position >= input.size() || input[position] != '}')
        return false;
    ++position;

    while (position < input.size() &&
           asciiWhitespace(input[position]))
    {
        ++position;
    }

    return position == input.size();
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
    const std::string& correlationId,
    const std::string& allow = "GET")
{
    ApiResponse response = problemResponse(
        405,
        "method_not_allowed",
        "Method not allowed",
        "The requested public API resource does not support this method.",
        path,
        requestId,
        correlationId);
    response.headers["Allow"] = allow;
    return response;
}

ApiResponse publicTimerCreateProblem(
    int statusCode,
    const std::string& code,
    const std::string& title,
    const std::string& detail,
    const std::string& path,
    const std::string& requestId,
    const std::string& correlationId)
{
    return problemResponse(
        statusCode,
        code,
        title,
        detail,
        path,
        requestId,
        correlationId);
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
    const bool timerCreateAdmissionAvailable,
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
        "\"},"
        "{\"id\":\"public-api.timer-create-admission\",\"version\":1,\"availability\":\"" +
        std::string(timerCreateAdmissionAvailable ? "available" : "unavailable") +
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

void PublicApiRuntime::registerTimerCreateAdmission(
    TimerCreateAdmission admission)
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    timerCreateAdmission_ = std::move(admission);
}

void PublicApiRuntime::resetTimerCreateAdmission()
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    timerCreateAdmission_ = TimerCreateAdmission{};
}

bool PublicApiRuntime::timerCreateAdmissionConfigured() const
{
    std::lock_guard<std::mutex> lock(
        timerCreateAdmissionMutex_);
    return static_cast<bool>(timerCreateAdmission_);
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
            timerCreateAdmissionConfigured(),
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
    ApiResponse& response,
    const std::string& body,
    const std::string& actorRef,
    const std::string& ifMatch,
    const std::string& idempotencyKey,
    const std::string& contentType,
    const std::string& authorizedBackendId) const
{
    const std::string path = requestPath(requestTarget);
    std::string operationId;
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

        if (!applicationJsonContentType(contentType))
        {
            response = publicTimerCreateProblem(
                415,
                "unsupported_media_type",
                "Unsupported media type",
                "Timer CREATE requires Content-Type application/json.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (body.size() > 4096U)
        {
            response = invalidRequestProblem(
                path,
                "The Timer CREATE request body is too large.",
                requestId,
                correlationId);
            return true;
        }

        JsonSyntaxValidator validator(body);
        if (!validator.valid())
        {
            response = publicTimerCreateProblem(
                400,
                "invalid_json",
                "Invalid JSON",
                "The Timer CREATE request body is not valid JSON.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (!emptyJsonObject(body))
        {
            response = publicTimerCreateProblem(
                422,
                "validation_error",
                "Validation failed",
                "Timer CREATE accepts a closed empty JSON object.",
                path,
                requestId,
                correlationId);
            return true;
        }

        if (ifMatch.empty())
        {
            response = publicTimerCreateProblem(
                428,
                "precondition_required",
                "Precondition required",
                "Timer CREATE requires one strong If-Match entity tag.",
                path,
                requestId,
                correlationId);
            return true;
        }

        std::string expectedAssignmentRevision;
        if (!vdrsuite::http::publicStrongEntityTagResourceRevision(
                ifMatch,
                expectedAssignmentRevision))
        {
            response = invalidRequestProblem(
                path,
                "If-Match must contain exactly one canonical strong VDR-Suite entity tag.",
                requestId,
                correlationId);
            return true;
        }

        if (!publicIdempotencyKeyValid(idempotencyKey))
        {
            response = invalidRequestProblem(
                path,
                idempotencyKey.empty()
                    ? "Idempotency-Key is required for Timer CREATE."
                    : "Idempotency-Key is malformed or too long.",
                requestId,
                correlationId);
            return true;
        }

        TimerCreateAdmission admission;
        {
            std::lock_guard<std::mutex> lock(
                timerCreateAdmissionMutex_);
            admission = timerCreateAdmission_;
        }

        if (!admission)
        {
            response = serviceUnavailableProblem(
                path,
                requestId,
                correlationId);
            return true;
        }

        PublicTimerCreateAdmissionRequest admissionRequest;
        admissionRequest.actorRef = actorRef;
        admissionRequest.backendId = authorizedBackendId;
        admissionRequest.timerAssignmentId = timerAssignmentId;
        admissionRequest.expectedAssignmentRevision =
            expectedAssignmentRevision;
        admissionRequest.idempotencyKey = idempotencyKey;

        const PublicTimerCreateAdmissionResult admitted =
            admission(admissionRequest);

        switch (admitted.status)
        {
            case PublicTimerCreateAdmissionStatus::accepted:
            case PublicTimerCreateAdmissionStatus::replayed:
            {
                if (admitted.operation.operationId.empty() ||
                    admitted.operation.state.empty() ||
                    admitted.operation.backendId !=
                        authorizedBackendId ||
                    admitted.operation.resourceRevision.empty())
                {
                    response = serviceUnavailableProblem(
                        path,
                        requestId,
                        correlationId);
                    return true;
                }

                const std::string operationPath =
                    std::string(PublicOperationPrefix) +
                    admitted.operation.operationId;
                response = publicOperationResponse(
                    admitted.operation,
                    operationPath,
                    requestId,
                    correlationId,
                    "");
                response.statusCode = 202;
                response.headers["Location"] = operationPath;
                return true;
            }

            case PublicTimerCreateAdmissionStatus::invalid:
                response = publicTimerCreateProblem(
                    422,
                    "validation_error",
                    "Validation failed",
                    "The Timer CREATE submission is not valid.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::notFound:
                response = notFoundProblem(
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::readOnlyBackend:
                response = publicTimerCreateProblem(
                    403,
                    "read_only_backend",
                    "Backend is read-only",
                    "The selected backend does not permit Timer mutation.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::backendUnavailable:
                response = publicTimerCreateProblem(
                    503,
                    "backend_unavailable",
                    "Backend unavailable",
                    "The selected backend cannot currently accept Timer mutation.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::revisionConflict:
                response = publicTimerCreateProblem(
                    412,
                    "revision_conflict",
                    "Resource revision conflict",
                    "The TimerAssignment changed after it was read.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::stateConflict:
                response = publicTimerCreateProblem(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The TimerAssignment state does not permit Timer CREATE.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::generationConflict:
                response = publicTimerCreateProblem(
                    409,
                    "generation_conflict",
                    "Backend generation conflict",
                    "The TimerAssignment backend generation is no longer current.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::idempotencyConflict:
                response = publicTimerCreateProblem(
                    409,
                    "idempotency_conflict",
                    "Idempotency conflict",
                    "Idempotency-Key was already used for a different Timer CREATE submission.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::operationConflict:
                response = publicTimerCreateProblem(
                    409,
                    "operation_conflict",
                    "Operation conflict",
                    "The durable operation state conflicts with Timer CREATE.",
                    path,
                    requestId,
                    correlationId);
                return true;

            case PublicTimerCreateAdmissionStatus::serviceUnavailable:
                response = serviceUnavailableProblem(
                    path,
                    requestId,
                    correlationId);
                return true;
        }
    }

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        publicOperationPath(path, operationId))
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

    if (publicTimerAssignmentPath(
            path,
            timerAssignmentId))
    {
        response = methodNotAllowedProblem(
            path,
            requestId,
            correlationId,
            "GET, POST");
        return true;
    }

    if (path == "/api/v1" ||
        path == "/api/v1/capabilities" ||
        publicOperationPath(path, operationId))
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
