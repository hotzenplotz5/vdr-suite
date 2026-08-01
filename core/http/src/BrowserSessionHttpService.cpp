#include "BrowserSessionHttpService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionIssuanceService.h"
#include "BrowserSessionLifecycleService.h"

#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

namespace
{
constexpr const char* CookieName = "vdr_suite_session";
constexpr const char* IssuePermission = "session.issue.self";
constexpr const char* IssueAction = "browser.session.issue";
constexpr const char* RevokePermission = "session.revoke.self";
constexpr const char* RevokeAction = "browser.session.revoke";

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

std::string nowUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
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

std::string sessionCookie(
    const std::string& cookieValue,
    int lifetimeSeconds)
{
    return std::string(CookieName) + "=" + cookieValue +
        "; Path=/; Max-Age=" + std::to_string(lifetimeSeconds) +
        "; HttpOnly; Secure; SameSite=Strict";
}

std::string expiredSessionCookie()
{
    return std::string(CookieName) +
        "=; Path=/; Max-Age=0; "
        "Expires=Thu, 01 Jan 1970 00:00:00 GMT; "
        "HttpOnly; Secure; SameSite=Strict";
}

HttpServerResponse accountabilityUnavailableResponse(
    const RequestSecurityContext& context,
    bool expireCookie)
{
    HttpServerResponse response = errorResponse(
        503,
        "accountability_unavailable",
        "Security outcome accountability persistence is unavailable",
        context);
    if (expireCookie)
    {
        response.headers["Set-Cookie"] = expiredSessionCookie();
    }
    return response;
}
}

BrowserSessionHttpService::BrowserSessionHttpService(
    BrowserSessionIssuanceService& issuanceService,
    BrowserSessionLifecycleService& lifecycleService,
    AccountabilityEventRepository& accountabilityRepository)
    : BrowserSessionHttpService(
          issuanceService,
          lifecycleService,
          accountabilityRepository,
          SecurityConfiguration::fromEnvironment().browserSessionLifetime)
{
}

BrowserSessionHttpService::BrowserSessionHttpService(
    BrowserSessionIssuanceService& issuanceService,
    BrowserSessionLifecycleService& lifecycleService,
    AccountabilityEventRepository& accountabilityRepository,
    BrowserSessionLifetimeConfiguration lifetimeConfiguration)
    : issuanceService_(issuanceService),
      lifecycleService_(lifecycleService),
      accountabilityRepository_(accountabilityRepository),
      lifetimeConfiguration_(std::move(lifetimeConfiguration))
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

    if (!lifetimeConfiguration_.valid())
    {
        if (!appendOutcome(
                context,
                false,
                IssuePermission,
                IssueAction,
                "browser_session_lifetime_configuration_invalid"))
        {
            return accountabilityUnavailableResponse(context, false);
        }
        return errorResponse(
            503,
            "browser_session_lifetime_configuration_invalid",
            "The browser session lifetime configuration is invalid",
            context);
    }

    BrowserSessionIssuanceRequest request;
    request.actorId = context.actor.actorId;
    request.deviceId = context.device->deviceId;
    request.issuedFromCredentialId = context.credential->credentialId;
    request.lifetimeSeconds = lifetimeConfiguration_.seconds;

    auto issued = issuanceService_.issue(request);
    if (!issued.has_value())
    {
        if (!appendOutcome(
                context,
                false,
                IssuePermission,
                IssueAction,
                "browser_session_issuance_failed"))
        {
            return accountabilityUnavailableResponse(context, false);
        }
        return errorResponse(
            503,
            "browser_session_issuance_failed",
            "The browser session could not be issued",
            context);
    }

    if (!appendOutcome(
            context,
            true,
            IssuePermission,
            IssueAction,
            "browser_session_issued",
            issued->sessionId))
    {
        lifecycleService_.revoke(
            issued->sessionId,
            issued->credentialId);
        issued->clearSecrets();
        return accountabilityUnavailableResponse(context, false);
    }

    HttpServerResponse response;
    response.statusCode = 200;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Set-Cookie"] =
        sessionCookie(
            issued->sessionCookieValue,
            lifetimeConfiguration_.seconds);
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
        if (!appendOutcome(
                context,
                false,
                RevokePermission,
                RevokeAction,
                "browser_session_revocation_failed"))
        {
            return accountabilityUnavailableResponse(context, false);
        }
        return errorResponse(
            503,
            "browser_session_revocation_failed",
            "The browser session could not be revoked",
            context);
    }

    if (!appendOutcome(
            context,
            true,
            RevokePermission,
            RevokeAction,
            "browser_session_revoked"))
    {
        return accountabilityUnavailableResponse(context, true);
    }

    HttpServerResponse response;
    response.statusCode = 204;
    response.headers["Cache-Control"] = "no-store";
    response.headers["Pragma"] = "no-cache";
    response.headers["Set-Cookie"] = expiredSessionCookie();
    return response;
}

bool BrowserSessionHttpService::appendOutcome(
    const RequestSecurityContext& context,
    bool succeeded,
    const std::string& permission,
    const std::string& action,
    const std::string& reasonCode,
    const std::string& sessionId) const
{
    AccountabilityEvent event;
    event.eventId = opaqueId("audit");
    event.classes = succeeded ? "audit" : "audit,security";
    event.eventType = succeeded
        ? "operation.succeeded"
        : "operation.failed";
    event.severity = succeeded ? "info" : "error";
    event.occurredAt = nowUtc();
    event.actorId = context.actor.actorId.empty()
        ? "anonymous"
        : context.actor.actorId;
    event.actorType = actorTypeName(context.actor.type);
    event.deviceId = context.device
        ? context.device->deviceId
        : "";
    event.sessionId = !sessionId.empty()
        ? sessionId
        : (context.session ? context.session->sessionId : "");
    event.authenticationState =
        authenticationStateName(context.authenticationState);
    event.permission = permission;
    event.backendId = "*";
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = action;
    event.decision = "allowed";
    event.reasonCode = reasonCode;
    event.outcome = succeeded ? "succeeded" : "failed";
    return accountabilityRepository_.append(event);
}

std::string BrowserSessionHttpService::opaqueId(
    const std::string& prefix) const
{
    const auto ticks =
        std::chrono::steady_clock::now().time_since_epoch().count();
    const unsigned long long sequence =
        idCounter_.fetch_add(1) + 1;
    std::ostringstream output;
    output << prefix << '-' << std::hex
           << static_cast<unsigned long long>(ticks)
           << '-' << sequence;
    return output.str();
}
