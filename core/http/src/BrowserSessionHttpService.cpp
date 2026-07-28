#include "BrowserSessionHttpService.h"

#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"

#include <string>

namespace
{
constexpr const char* CookieName = "vdr_suite_session";

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
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) +
        "\",\"requestId\":\"" + jsonEscape(context.requestId) +
        "\"}}";
    return response;
}

std::string sessionCookie(const std::string& cookieValue)
{
    return std::string(CookieName) + "=" + cookieValue +
        "; Path=/; Max-Age=" +
        std::to_string(BrowserSessionIssuanceService::DefaultLifetimeSeconds) +
        "; HttpOnly; Secure; SameSite=Strict";
}

std::string expiredSessionCookie()
{
    return std::string(CookieName) +
        "=; Path=/; Max-Age=0; "
        "Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
        "HttpOnly; Secure; SameSite=Strict";
}
}

BrowserSessionHttpService::BrowserSessionHttpService(
    BrowserSessionIssuanceService& issuanceService,
    BrowserSessionLifecycleService& lifecycleService)
    : issuanceService_(issuanceService),
      lifecycleService_(lifecycleService)
{
}

HttpServerResponse BrowserSessionHttpService::login(
    const RequestSecurityContext& context)
{
    if (!context.authenticated() ||
        context.actor.actorId.empty() ||
        !context.device.has_value() ||
        context.device->deviceId.empty() ||
        !context.credential.has_value() ||
        context.credential->credentialId.empty())
    {
        return errorResponse(
            401,
            "invalid_session_issuance_context",
            "An active authenticated credential is required",
            context);
    }

    BrowserSessionIssuanceRequest request;
    request.actorId = context.actor.actorId;
    request.deviceId = context.device->deviceId;
    request.issuedFromCredentialId = context.credential->credentialId;
    request.lifetimeSeconds =
        BrowserSessionIssuanceService::DefaultLifetimeSeconds;

    auto issued = issuanceService_.issue(request);
    if (!issued.has_value())
    {
        return errorResponse(
            503,
            "browser_session_issuance_failed",
            "The browser session could not be issued",
            context);
    }

    HttpServerResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Set-Cookie"] =
        sessionCookie(issued->sessionCookieValue);
    response.body =
        "{\"csrfToken\":\"" + jsonEscape(issued->csrfToken) +
        "\",\"expiresAt\":\"" + jsonEscape(issued->expiresAt) +
        "\",\"requestId\":\"" + jsonEscape(context.requestId) +
        "\"}";
    issued->clearSecrets();
    return response;
}

HttpServerResponse BrowserSessionHttpService::logout(
    const RequestSecurityContext& context)
{
    if (!context.authenticated() ||
        !context.session.has_value() ||
        context.session->sessionId.empty() ||
        !context.credential.has_value() ||
        context.credential->credentialId.empty())
    {
        return errorResponse(
            401,
            "invalid_browser_session_context",
            "An active browser session is required",
            context);
    }

    if (!lifecycleService_.revoke(
            context.session->sessionId,
            context.credential->credentialId))
    {
        return errorResponse(
            503,
            "browser_session_revocation_failed",
            "The browser session could not be revoked",
            context);
    }

    HttpServerResponse response;
    response.statusCode = 204;
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Set-Cookie"] = expiredSessionCookie();
    return response;
}
