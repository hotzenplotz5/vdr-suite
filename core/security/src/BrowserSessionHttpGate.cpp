#include "BrowserSessionHttpGate.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "BrowserSessionAuthenticator.h"
#include "BrowserSessionCredentialRepository.h"
#include "SecurityPermissionGrantRepository.h"
#include "LegacyBasicAuthenticator.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr const char* LoginPath = "/api/security/browser-sessions";
constexpr const char* LogoutPath = "/api/security/browser-sessions/logout";

std::string lowerAscii(std::string value)
{
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos
        ? target
        : target.substr(0, query);
}

std::string headerValue(
    const HttpServerRequest& request,
    const std::string& name)
{
    const std::string wanted = lowerAscii(name);
    for (const auto& header : request.headers)
    {
        if (lowerAscii(header.first) == wanted)
        {
            return header.second;
        }
    }
    return {};
}

bool safeContextToken(const std::string& value)
{
    if (value.empty() || value.size() > 128)
    {
        return false;
    }

    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character)
        {
            return std::isalnum(character) ||
                character == '-' ||
                character == '_' ||
                character == '.' ||
                character == ':';
        });
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

std::string nowUtc()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    std::ostringstream output;
    output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return output.str();
}

std::string authenticationReason(
    const RequestSecurityContext& context)
{
    if (context.authenticationState == AuthenticationState::Anonymous)
    {
        return "authentication_required";
    }
    if (context.authenticationState == AuthenticationState::Invalid)
    {
        return "invalid_credentials";
    }
    if (!context.actor.active || context.actor.actorId.empty())
    {
        return "actor_revoked";
    }
    if (context.device.has_value() && !context.device->active)
    {
        return "device_revoked";
    }
    if (context.credential.has_value())
    {
        if (context.credential->revoked || !context.credential->active)
        {
            return "credential_revoked";
        }
        if (context.credential->expired)
        {
            return "credential_expired";
        }
    }
    if (context.session.has_value())
    {
        if (context.session->revoked || !context.session->active)
        {
            return "session_revoked";
        }
        if (context.session->expired)
        {
            return "session_expired";
        }
    }
    if (context.authenticationState == AuthenticationState::Expired)
    {
        return "session_expired";
    }
    if (context.authenticationState == AuthenticationState::Revoked)
    {
        return "session_revoked";
    }
    return "invalid_credentials";
}

std::string authenticationMessage(const std::string& reasonCode)
{
    if (reasonCode == "authentication_required")
    {
        return "Authentication is required";
    }
    if (reasonCode == "invalid_credentials")
    {
        return "The supplied credentials are invalid";
    }
    if (reasonCode == "session_expired" ||
        reasonCode == "credential_expired")
    {
        return "The authenticated session has expired";
    }
    return "The authenticated identity is no longer active";
}
}

BrowserSessionHttpGate::BrowserSessionHttpGate(
    SecurityConfiguration configuration,
    AccountabilityEventRepository& accountabilityRepository,
    const BrowserSessionCredentialRepository& credentialRepository,
    const SecurityPermissionGrantRepository& grantRepository,
    const PersistentIdentityResolver* persistentIdentityResolver,
    const ManagedBasicAuthenticator* managedBasicAuthenticator)
    : configuration_(std::move(configuration)),
      accountabilityRepository_(accountabilityRepository),
      persistentIdentityResolver_(persistentIdentityResolver),
      managedBasicAuthenticator_(managedBasicAuthenticator),
      legacyAuthenticator_(
          std::make_unique<LegacyBasicAuthenticator>(configuration_)),
      browserAuthenticator_(
          std::make_unique<BrowserSessionAuthenticator>(
              credentialRepository,
              grantRepository,
              configuration_.browserSessionIdle.valid()
                  ? configuration_.browserSessionIdle.timeoutSeconds
                  : -1,
              BrowserSessionIdleConfiguration::LastSeenWriteIntervalSeconds))
{
}

BrowserSessionHttpGate::~BrowserSessionHttpGate() = default;

bool BrowserSessionHttpGate::handles(
    const HttpServerRequest& request) const
{
    if (request.method != "POST")
    {
        return false;
    }

    const std::string path = requestPath(request.path);
    return path == LoginPath || path == LogoutPath;
}

BrowserSessionGateDecision BrowserSessionHttpGate::evaluate(
    const HttpServerRequest& request) const
{
    BrowserSessionGateDecision gate;
    const std::string path = requestPath(request.path);
    gate.login = request.method == "POST" && path == LoginPath;
    gate.logout = request.method == "POST" && path == LogoutPath;

    if (!gate.login && !gate.logout)
    {
        gate.context = requestContextSeed(request);
        gate.rejection = errorResponse(
            404,
            "browser_session_route_not_found",
            "The browser session route is not available",
            gate.context,
            false);
        return gate;
    }

    const std::string permission = gate.login
        ? "session.issue.self"
        : "session.revoke.self";
    const std::string action = gate.login
        ? "browser.session.issue"
        : "browser.session.revoke";

    if (!configuration_.browserSessionIdle.valid())
    {
        gate.context = requestContextSeed(request);
        if (!appendDecisionEvent(
                gate.context,
                false,
                permission,
                action,
                "browser_session_idle_configuration_invalid"))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context,
                false);
            return gate;
        }

        gate.rejection = errorResponse(
            503,
            "browser_session_idle_configuration_invalid",
            "The browser session idle configuration is invalid",
            gate.context,
            false);
        return gate;
    }

    gate.context = gate.login
        ? authenticateBasic(request)
        : authenticateBrowser(request);

    if (gate.logout &&
        gate.context.authenticated() &&
        gate.context.permissionGrantResolution ==
            PermissionGrantResolutionState::Unavailable)
    {
        if (!appendDecisionEvent(
                gate.context,
                false,
                permission,
                action,
                "browser_session_activity_unavailable"))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context,
                false);
            return gate;
        }

        gate.rejection = errorResponse(
            503,
            "browser_session_activity_unavailable",
            "Browser session activity persistence is unavailable",
            gate.context,
            false);
        return gate;
    }

    if (!gate.context.authenticated())
    {
        const std::string reasonCode =
            authenticationReason(gate.context);
        if (!appendDecisionEvent(
                gate.context,
                false,
                permission,
                action,
                reasonCode))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context,
                false);
            return gate;
        }

        gate.rejection = errorResponse(
            401,
            reasonCode,
            authenticationMessage(reasonCode),
            gate.context,
            false);
        return gate;
    }

    if (gate.logout &&
        (!browserAuthenticator_ ||
         !browserAuthenticator_->verifyCsrf(request.headers)))
    {
        if (!appendDecisionEvent(
                gate.context,
                false,
                permission,
                action,
                "csrf_validation_failed"))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context,
                false);
            return gate;
        }

        gate.rejection = errorResponse(
            403,
            "csrf_validation_failed",
            "A valid CSRF token is required",
            gate.context,
            false);
        return gate;
    }

    if (!appendDecisionEvent(
            gate.context,
            true,
            permission,
            action,
            "self_service_session_lifecycle_allowed"))
    {
        gate.rejection = errorResponse(
            503,
            "accountability_unavailable",
            "Security accountability persistence is unavailable",
            gate.context,
            false);
        return gate;
    }

    gate.allowed = true;
    return gate;
}

RequestSecurityContext BrowserSessionHttpGate::authenticateBasic(
    const HttpServerRequest& request) const
{
    const RequestSecurityContext seed = requestContextSeed(request);
    RequestSecurityContext context = legacyAuthenticator_->authenticate(
        request.headers,
        seed.requestId,
        seed.correlationId);
    if (context.authenticated())
    {
        return resolvePersistentIdentity(std::move(context));
    }

    if (managedBasicAuthenticator_ != nullptr)
    {
        RequestSecurityContext managedContext =
            managedBasicAuthenticator_->authenticate(
                request.headers,
                seed.requestId,
                seed.correlationId);
        if (managedContext.authenticated())
        {
            return resolvePersistentIdentity(
                std::move(managedContext));
        }
    }

    return context;
}

RequestSecurityContext BrowserSessionHttpGate::authenticateBrowser(
    const HttpServerRequest& request) const
{
    const RequestSecurityContext seed = requestContextSeed(request);
    if (!browserAuthenticator_)
    {
        return seed;
    }

    RequestSecurityContext context = browserAuthenticator_->authenticate(
        request.headers,
        seed.requestId,
        seed.correlationId);
    if (context.authenticated())
    {
        return resolvePersistentIdentity(std::move(context));
    }
    return context;
}

RequestSecurityContext BrowserSessionHttpGate::resolvePersistentIdentity(
    RequestSecurityContext context) const
{
    if (persistentIdentityResolver_ != nullptr)
    {
        context = persistentIdentityResolver_->resolve(
            std::move(context));
    }
    return context;
}

RequestSecurityContext BrowserSessionHttpGate::requestContextSeed(
    const HttpServerRequest& request) const
{
    RequestSecurityContext context;
    context.requestId = headerValue(request, "X-Request-ID");
    if (!safeContextToken(context.requestId))
    {
        context.requestId = opaqueId("req");
    }

    context.correlationId = headerValue(request, "X-Correlation-ID");
    if (!context.correlationId.empty() &&
        !safeContextToken(context.correlationId))
    {
        context.correlationId.clear();
    }
    return context;
}

bool BrowserSessionHttpGate::appendDecisionEvent(
    const RequestSecurityContext& context,
    bool allowed,
    const std::string& permission,
    const std::string& action,
    const std::string& reasonCode) const
{
    AccountabilityEvent event;
    event.eventId = opaqueId("audit");
    event.classes = allowed ? "audit" : "audit,security";
    event.eventType = allowed
        ? "authorization.allowed"
        : "authorization.denied";
    event.severity = allowed ? "info" : "warning";
    event.occurredAt = nowUtc();
    event.actorId = context.actor.actorId.empty()
        ? "anonymous"
        : context.actor.actorId;
    event.actorType = actorTypeName(context.actor.type);
    event.deviceId = context.device
        ? context.device->deviceId
        : "";
    event.sessionId = context.session
        ? context.session->sessionId
        : "";
    event.authenticationState =
        authenticationStateName(context.authenticationState);
    event.permission = permission;
    event.backendId = "*";
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = action;
    event.decision = allowed ? "allowed" : "denied";
    event.reasonCode = reasonCode;
    event.outcome = allowed
        ? "dispatch_authorized"
        : "dispatch_denied";
    return accountabilityRepository_.append(event);
}

HttpServerResponse BrowserSessionHttpGate::errorResponse(
    int statusCode,
    const std::string& code,
    const std::string& message,
    const RequestSecurityContext& context,
    bool advertiseBasic) const
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    if (advertiseBasic)
    {
        response.headers["WWW-Authenticate"] =
            "Basic realm=\"VDR-Suite\", charset=\"UTF-8\"";
    }
    response.headers["X-Request-ID"] = context.requestId;
    if (!context.correlationId.empty())
    {
        response.headers["X-Correlation-ID"] = context.correlationId;
    }
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"" + jsonEscape(message) +
        "\",\"requestId\":\"" + jsonEscape(context.requestId) +
        "\"}}";
    return response;
}

std::string BrowserSessionHttpGate::opaqueId(
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
