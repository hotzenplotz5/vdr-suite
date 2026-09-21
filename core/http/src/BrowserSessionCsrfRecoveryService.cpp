#include "BrowserSessionCsrfRecoveryService.h"

#include "BrowserSessionCredentialRepository.h"
#include "BrowserSessionCsrfToken.h"

#include <algorithm>
#include <string>

namespace
{
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

HttpServerResponse errorResponse(
    int statusCode,
    const std::string& code,
    const std::string& message,
    const RequestSecurityContext& context)
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Vary"] = "Cookie";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) +
        "\",\"requestId\":\"" + jsonEscape(context.requestId) +
        "\"}}";
    return response;
}
}

BrowserSessionCsrfRecoveryService::BrowserSessionCsrfRecoveryService(
    const BrowserSessionCredentialRepository& repository)
    : repository_(repository)
{
}

HttpServerResponse BrowserSessionCsrfRecoveryService::recover(
    const RequestSecurityContext& context) const
{
    if (!context.authenticated() ||
        !context.session.has_value() ||
        context.session->sessionId.empty())
    {
        return errorResponse(
            401,
            "authentication_required",
            "An active browser session is required",
            context);
    }

    const auto record = repository_.findBySessionId(
        context.session->sessionId);
    if (!record.has_value() ||
        record->actorId != context.actor.actorId ||
        !record->active ||
        record->revoked)
    {
        return errorResponse(
            401,
            "session_revoked",
            "The browser session is no longer active",
            context);
    }
    if (record->expired || context.session->expired)
    {
        return errorResponse(
            401,
            "session_expired",
            "The browser session has expired",
            context);
    }
    if (!BrowserSessionCredentialRepository::supportsSecretHash(
            record->csrfSecretHash))
    {
        return errorResponse(
            503,
            "csrf_recovery_unavailable",
            "The browser session security proof cannot be recovered",
            context);
    }

    std::string csrfToken =
        BrowserSessionCsrfToken::derive(record->csrfSecretHash);
    if (csrfToken.empty())
    {
        return errorResponse(
            503,
            "csrf_recovery_unavailable",
            "The browser session security proof cannot be recovered",
            context);
    }

    HttpServerResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Vary"] = "Cookie";
    response.body =
        "{\"csrfToken\":\"" + jsonEscape(csrfToken) +
        "\",\"expiresAt\":\"" + jsonEscape(record->expiresAt) +
        "\",\"requestId\":\"" + jsonEscape(context.requestId) +
        "\"}";

    std::fill(csrfToken.begin(), csrfToken.end(), '\0');
    csrfToken.clear();
    return response;
}
