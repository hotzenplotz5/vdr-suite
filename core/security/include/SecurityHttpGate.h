#pragma once

#include "AccountabilityEventRepository.h"
#include "AuthorizationService.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "LegacyBasicAuthenticator.h"
#include "SecurityConfiguration.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>

struct SecurityGateDecision
{
    bool allowed = false;
    bool protectedMutation = false;
    RequestSecurityContext context;
    HttpServerResponse rejection;
};

class SecurityHttpGate
{
public:
    SecurityHttpGate(
        SecurityConfiguration configuration,
        AccountabilityEventRepository& accountabilityRepository)
        : configuration_(std::move(configuration)),
          authenticator_(configuration_),
          accountabilityRepository_(accountabilityRepository)
    {
    }

    SecurityGateDecision evaluate(const HttpServerRequest& request) const
    {
        SecurityGateDecision gate;
        gate.context = authenticate(request);

        if (configuration_.mode == SecurityMode::LegacyBasicCompatibility &&
            !gate.context.authenticated())
        {
            AuthorizationDecision decision;
            decision.reasonCode =
                gate.context.authenticationState == AuthenticationState::Invalid
                    ? "invalid_credentials"
                    : "authentication_required";
            decision.action = "http.access";
            decision.permission = "legacy.compatibility.access";
            decision.backendId = "*";
            return rejectWithAudit(
                gate,
                decision,
                401,
                messageForReason(decision.reasonCode),
                "",
                true);
        }

        const bool isPost = request.method == "POST";
        const bool isRemoteAction =
            isPost && requestPath(request.path) == "/api/vdr/remote/actions";

        if (!isRemoteAction)
        {
            if (configuration_.mode == SecurityMode::Enforced && isPost)
            {
                AuthorizationDecision decision;
                decision.reasonCode = "security_policy_not_migrated";
                decision.permission = "unmapped.mutation";
                decision.backendId = "*";
                decision.action = "http.mutation";
                return rejectWithAudit(
                    gate,
                    decision,
                    503,
                    "This mutation route has not yet been migrated to the Phase 62 security contract",
                    "");
            }

            gate.allowed = true;
            return gate;
        }

        gate.protectedMutation = true;
        AuthorizationRequest requestToAuthorize;
        requestToAuthorize.permission = "remote.control";
        requestToAuthorize.backendId = jsonStringValue(request.body, "backendId");
        requestToAuthorize.action = "remote.control";

        const AuthorizationDecision decision =
            authorizationService_.authorize(gate.context, requestToAuthorize);
        const std::string operationId =
            jsonStringValue(request.body, "operationId");

        if (!appendDecisionEvent(gate.context, decision, operationId))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context);
            return gate;
        }

        if (!decision.allowed)
        {
            const int statusCode = decision.reasonCode == "invalid_backend_scope"
                ? 400
                : (authenticationFailure(decision) ? 401 : 403);
            gate.rejection = errorResponse(
                statusCode,
                decision.reasonCode,
                messageForReason(decision.reasonCode),
                gate.context,
                authenticationFailure(decision));
            return gate;
        }

        gate.allowed = true;
        return gate;
    }

    void decorateResponse(
        const RequestSecurityContext& context,
        HttpServerResponse& response) const
    {
        response.headers["X-Request-ID"] = context.requestId;
        if (!context.correlationId.empty())
        {
            response.headers["X-Correlation-ID"] = context.correlationId;
        }
    }

private:
    static std::string lowerAscii(std::string value)
    {
        std::transform(
            value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    static std::string requestPath(const std::string& target)
    {
        const std::size_t query = target.find('?');
        return query == std::string::npos ? target : target.substr(0, query);
    }

    static std::string headerValue(
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
        return "";
    }

    static std::string jsonStringValue(
        const std::string& body,
        const std::string& field)
    {
        const std::string needle = "\"" + field + "\"";
        std::size_t position = body.find(needle);
        if (position == std::string::npos)
        {
            return "";
        }
        position = body.find(':', position + needle.size());
        if (position == std::string::npos)
        {
            return "";
        }
        do
        {
            ++position;
        }
        while (position < body.size() &&
               std::isspace(static_cast<unsigned char>(body[position])));
        if (position >= body.size() || body[position] != '"')
        {
            return "";
        }

        ++position;
        std::string value;
        while (position < body.size())
        {
            const char character = body[position++];
            if (character == '"')
            {
                return value;
            }
            if (character != '\\')
            {
                value.push_back(character);
                continue;
            }
            if (position >= body.size())
            {
                return "";
            }
            switch (body[position++])
            {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: return "";
            }
        }
        return "";
    }

    static std::string jsonEscape(const std::string& value)
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

    static bool safeContextToken(const std::string& value)
    {
        if (value.empty() || value.size() > 128)
        {
            return false;
        }
        return std::all_of(
            value.begin(), value.end(),
            [](unsigned char character)
            {
                return std::isalnum(character) || character == '-' ||
                    character == '_' || character == '.' || character == ':';
            });
    }

    static bool authenticationFailure(const AuthorizationDecision& decision)
    {
        return decision.reasonCode == "authentication_required" ||
            decision.reasonCode == "invalid_credentials" ||
            decision.reasonCode == "session_expired" ||
            decision.reasonCode == "session_revoked" ||
            decision.reasonCode == "actor_revoked" ||
            decision.reasonCode == "device_revoked";
    }

    static std::string messageForReason(const std::string& reasonCode)
    {
        if (reasonCode == "authentication_required") return "Authentication is required";
        if (reasonCode == "invalid_credentials") return "The supplied credentials are invalid";
        if (reasonCode == "session_expired") return "The authenticated session has expired";
        if (reasonCode == "session_revoked" || reasonCode == "actor_revoked" ||
            reasonCode == "device_revoked")
        {
            return "The authenticated identity is no longer active";
        }
        if (reasonCode == "backend_scope_denied")
        {
            return "The actor is not permitted to mutate this backend";
        }
        if (reasonCode == "permission_denied")
        {
            return "The actor lacks the required permission";
        }
        if (reasonCode == "invalid_backend_scope")
        {
            return "A valid backend scope is required";
        }
        return "The request is not authorized";
    }

    static std::string nowUtc()
    {
        const std::time_t now = std::time(nullptr);
        std::tm utc{};
#if defined(_WIN32)
        gmtime_s(&utc, &now);
#else
        gmtime_r(&now, &utc);
#endif
        std::ostringstream output;
        output << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
        return output.str();
    }

    std::string opaqueId(const std::string& prefix) const
    {
        const auto ticks =
            std::chrono::steady_clock::now().time_since_epoch().count();
        const unsigned long long sequence = idCounter_.fetch_add(1) + 1;
        std::ostringstream output;
        output << prefix << '-' << std::hex
               << static_cast<unsigned long long>(ticks) << '-' << sequence;
        return output.str();
    }

    RequestSecurityContext authenticate(const HttpServerRequest& request) const
    {
        std::string requestId = headerValue(request, "X-Request-ID");
        if (!safeContextToken(requestId))
        {
            requestId = opaqueId("req");
        }
        std::string correlationId = headerValue(request, "X-Correlation-ID");
        if (!correlationId.empty() && !safeContextToken(correlationId))
        {
            correlationId.clear();
        }
        return authenticator_.authenticate(
            request.headers, requestId, correlationId);
    }

    bool appendDecisionEvent(
        const RequestSecurityContext& context,
        const AuthorizationDecision& decision,
        const std::string& operationId) const
    {
        AccountabilityEvent event;
        event.eventId = opaqueId("audit");
        event.classes = decision.allowed ? "audit" : "audit,security";
        event.eventType = decision.allowed
            ? "authorization.allowed" : "authorization.denied";
        event.severity = decision.allowed ? "info" : "warning";
        event.occurredAt = nowUtc();
        event.actorId = context.actor.actorId.empty()
            ? "anonymous" : context.actor.actorId;
        event.actorType = actorTypeName(context.actor.type);
        event.deviceId = context.device ? context.device->deviceId : "";
        event.sessionId = context.session ? context.session->sessionId : "";
        event.authenticationState =
            authenticationStateName(context.authenticationState);
        event.permission = decision.permission;
        event.backendId = decision.backendId;
        event.operationId = operationId;
        event.requestId = context.requestId;
        event.correlationId = context.correlationId;
        event.action = decision.action;
        event.decision = decision.allowed ? "allowed" : "denied";
        event.reasonCode = decision.reasonCode;
        event.outcome = decision.allowed
            ? "dispatch_authorized" : "dispatch_denied";
        return accountabilityRepository_.append(event);
    }

    SecurityGateDecision rejectWithAudit(
        SecurityGateDecision gate,
        const AuthorizationDecision& decision,
        int statusCode,
        const std::string& message,
        const std::string& operationId,
        bool advertiseBasic = false) const
    {
        if (!appendDecisionEvent(gate.context, decision, operationId))
        {
            gate.rejection = errorResponse(
                503,
                "accountability_unavailable",
                "Security accountability persistence is unavailable",
                gate.context);
            return gate;
        }
        gate.rejection = errorResponse(
            statusCode,
            decision.reasonCode,
            message,
            gate.context,
            advertiseBasic);
        return gate;
    }

    HttpServerResponse errorResponse(
        int statusCode,
        const std::string& code,
        const std::string& message,
        const RequestSecurityContext& context,
        bool advertiseBasic = false) const
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
        response.body =
            "{\"error\":{\"code\":\"" + jsonEscape(code) +
            "\",\"message\":\"" + jsonEscape(message) +
            "\",\"requestId\":\"" + jsonEscape(context.requestId) + "\"}}";
        decorateResponse(context, response);
        return response;
    }

    SecurityConfiguration configuration_;
    LegacyBasicAuthenticator authenticator_;
    AuthorizationService authorizationService_;
    AccountabilityEventRepository& accountabilityRepository_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};
