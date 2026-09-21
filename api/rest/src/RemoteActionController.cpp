#include "RemoteActionController.h"

#include <sstream>

namespace
{
std::string escapeJson(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size() + 8);

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
                if (character < 0x20)
                {
                    static const char* digits = "0123456789abcdef";
                    escaped += "\\u00";
                    escaped += digits[(character >> 4) & 0x0f];
                    escaped += digits[character & 0x0f];
                }
                else
                {
                    escaped += static_cast<char>(character);
                }
        }
    }

    return escaped;
}
}

std::string RemoteActionResultJsonSerializer::serialize(
    const RemoteActionResult& result) const
{
    std::ostringstream json;
    json << "{"
         << "\"success\":" << (result.success ? "true" : "false")
         << ",\"backendId\":\"" << escapeJson(result.backendId) << "\""
         << ",\"operationId\":\"" << escapeJson(result.operationId) << "\""
         << ",\"action\":\"" << escapeJson(remoteActionTypeName(result.action)) << "\""
         << ",\"failureKind\":\"" << escapeJson(remoteActionFailureKindName(result.failureKind)) << "\""
         << ",\"message\":\"" << escapeJson(result.message) << "\""
         << ",\"backendStatusCode\":" << result.backendStatusCode
         << ",\"errors\":[";

    for (std::size_t index = 0; index < result.errors.size(); ++index)
    {
        if (index > 0)
        {
            json << ',';
        }

        json << "\"" << escapeJson(result.errors[index]) << "\"";
    }

    json << "]}";
    return json.str();
}

RemoteActionController::RemoteActionController(
    const RemoteActionRequestParser& parser,
    const RemoteActionService& service,
    const RemoteActionResultJsonSerializer& serializer,
    SuccessCallback successCallback)
    : parser_(parser),
      service_(service),
      serializer_(serializer),
      successCallback_(std::move(successCallback))
{
}

int RemoteActionController::statusCodeFor(
    const RemoteActionResult& result)
{
    if (result.success)
    {
        return 200;
    }

    switch (result.failureKind)
    {
        case RemoteActionFailureKind::Validation: return 400;
        case RemoteActionFailureKind::BackendNotFound: return 404;
        case RemoteActionFailureKind::Permission: return 403;
        case RemoteActionFailureKind::Capability: return 409;
        case RemoteActionFailureKind::ExecutorUnavailable: return 503;
        case RemoteActionFailureKind::Transport: return 502;
        case RemoteActionFailureKind::BackendRejected: return 422;
        case RemoteActionFailureKind::BackendFailure: return 502;
        case RemoteActionFailureKind::None: return 500;
    }

    return 500;
}

ApiResponse RemoteActionController::executeBody(
    const std::string& body) const
{
    const RemoteActionParseResult parsed = parser_.parse(body);
    RemoteActionResult result;

    if (!parsed.validJson)
    {
        result = RemoteActionResult::failed(
            parsed.request,
            RemoteActionFailureKind::Validation,
            "Remote action request JSON is invalid",
            parsed.errors);
    }
    else
    {
        result = service_.execute(parsed.request);
    }

    if (result.success && successCallback_)
    {
        successCallback_(parsed.request);
    }

    ApiResponse response;
    response.statusCode = statusCodeFor(result);
    response.contentType = "application/json";
    response.body = serializer_.serialize(result);
    return response;
}
