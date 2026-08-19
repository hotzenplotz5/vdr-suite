#pragma once

#include "AccountabilityEventRepository.h"
#include "AuthorizationService.h"
#include "BrowserSessionAuthenticator.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "LegacyBasicAuthenticator.h"
#include "ManagedBasicAuthenticator.h"
#include "PersistentIdentityResolver.h"
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
    bool browserSessionPresented = false;
    bool browserAuthenticated = false;
    AuthorizationDecision authorizationDecision;
    std::string operationId;
    RequestSecurityContext context;
    HttpServerResponse rejection;
};

class SecurityHttpGate
{
private:
    struct AuthenticationResult
    {
        RequestSecurityContext context;
        bool browserSessionPresented = false;
        bool browserAuthenticated = false;
    };

public:
    SecurityHttpGate(
        SecurityConfiguration configuration,
        AccountabilityEventRepository& accountabilityRepository,
        const PersistentIdentityResolver* persistentIdentityResolver = nullptr,
        const ManagedBasicAuthenticator* managedBasicAuthenticator = nullptr,
        const BrowserSessionAuthenticator* browserSessionAuthenticator = nullptr)
        : configuration_(std::move(configuration)),
          legacyAuthenticator_(configuration_),
          accountabilityRepository_(accountabilityRepository),
          persistentIdentityResolver_(persistentIdentityResolver),
          managedBasicAuthenticator_(managedBasicAuthenticator),
          browserSessionAuthenticator_(browserSessionAuthenticator)
    {
    }

    SecurityGateDecision evaluate(const HttpServerRequest& request) const
    {
        SecurityGateDecision gate;
        AuthenticationResult authentication = authenticate(request);
        gate.browserSessionPresented = authentication.browserSessionPresented;
        gate.browserAuthenticated = authentication.browserAuthenticated;
        gate.context = std::move(authentication.context);

        if (configuration_.mode == SecurityMode::LegacyBasicCompatibility &&
            !gate.context.authenticated())
        {
            return rejectAuthentication(gate);
        }

        if (configuration_.mode == SecurityMode::Enforced &&
            gate.context.authenticationState != AuthenticationState::Anonymous &&
            !gate.context.authenticated())
        {
            return rejectAuthentication(gate);
        }

        if (gate.browserAuthenticated &&
            gate.context.permissionGrantResolution ==
                PermissionGrantResolutionState::Unavailable)
        {
            AuthorizationDecision decision;
            decision.reasonCode = "permission_grants_unavailable";
            decision.permission = "security.permissions.resolve";
            decision.backendId = "*";
            decision.action = "http.browser.access";
            return rejectWithAudit(
                gate,
                decision,
                503,
                "Browser permission persistence is unavailable",
                "");
        }

        const bool isPost = request.method == "POST";
        const std::string path = requestPath(request.path);
        const bool isRemoteAction = isPost && path == "/api/vdr/remote/actions";
        const bool isTimerCreateAction = isPost && path == "/api/vdr/timers/actions/create";
        const bool isTimerUpdateAction = isPost && path == "/api/vdr/timers/actions/update";
        const bool isTimerDeleteAction = isPost && path == "/api/vdr/timers/actions/delete";
        const bool isChannelMoveAction = isPost &&
            (path == "/api/vdr/channels/move" || path == "/api/vdr/channels/actions/move");
        const bool isRecordingExecutionAction = isPost &&
            (path == "/api/recordings/actions/execute" || path == "/api/vdr/recordings/actions/execute");
        const bool isSearchTimerCreateAction = isPost &&
            (path == "/api/searchtimers" || path == "/api/vdr/searchtimers");
        const bool isSearchTimerUpdateAction = isPost &&
            (path == "/api/searchtimers/update" || path == "/api/vdr/searchtimers/update");
        const bool isSearchTimerDeleteAction = isPost &&
            (path == "/api/searchtimers/delete" || path == "/api/vdr/searchtimers/delete");
        const bool isSearchTimerExecuteAction = isPost &&
            (path == "/api/searchtimers/execute" || path == "/api/vdr/searchtimers/execute");
        const bool isSearchTimerRealTestAction = isPost &&
            (path == "/api/searchtimers/real-test" || path == "/api/vdr/searchtimers/real-test");
        const bool isSearchTimerPreviewCacheRefreshAction = isPost &&
            (path == "/api/searchtimers/preview/cache/refresh" || path == "/api/vdr/searchtimers/preview/cache/refresh");
        const bool isEpgCacheRefreshAction = isPost && path == "/api/epg/cache/refresh";
        const bool isNativeFuzzyRefreshAction = isPost &&
            (path == "/api/epgsearch/native-fuzzy/refresh" || path == "/api/vdr/epgsearch/native-fuzzy/refresh");
        const bool isNativeFuzzyStaleProbeDeleteAction = isPost &&
            (path == "/api/epgsearch/native-fuzzy/stale-probes/delete" ||
             path == "/api/vdr/epgsearch/native-fuzzy/stale-probes/delete");
        const bool isSeriesArtworkSettingsAction =
            isPost &&
            path.size() > std::string("/api/backends/").size() +
                std::string("/settings/series-artwork").size() &&
            path.compare(0, std::string("/api/backends/").size(), "/api/backends/") == 0 &&
            path.compare(
                path.size() - std::string("/settings/series-artwork").size(),
                std::string("/settings/series-artwork").size(),
                "/settings/series-artwork") == 0;
        std::string manualMetadataBackendId;
        std::string manualMetadataOperation;
        const bool isManualRecordingMetadataAction = isPost &&
            manualRecordingMetadataRoute(path, manualMetadataBackendId, manualMetadataOperation);
        const bool isMediaSessionMutation = isPost && path == "/api/media/sessions";
        const bool isRecordingPlaybackSessionCreate = isMediaSessionMutation;
        const bool isSafePost = isPost &&
            (path == "/api/recordings/actions/validate" ||
             path == "/api/vdr/recordings/actions/validate" ||
             path == "/api/recordings/actions/preview" ||
             path == "/api/vdr/recordings/actions/preview" ||
             path == "/api/searchtimers/validate" ||
             path == "/api/vdr/searchtimers/validate" ||
             path == "/api/searchtimers/plan" ||
             path == "/api/vdr/searchtimers/plan");
        const bool isProtectedMutation =
            isRemoteAction || isTimerCreateAction || isTimerUpdateAction ||
            isTimerDeleteAction || isChannelMoveAction || isRecordingExecutionAction ||
            isSearchTimerCreateAction || isSearchTimerUpdateAction || isSearchTimerDeleteAction ||
            isSearchTimerExecuteAction || isSearchTimerRealTestAction ||
            isSearchTimerPreviewCacheRefreshAction || isEpgCacheRefreshAction ||
            isNativeFuzzyRefreshAction || isNativeFuzzyStaleProbeDeleteAction ||
            isSeriesArtworkSettingsAction || isManualRecordingMetadataAction;
        const bool isExplicitlyAuthorizedPost =
            isProtectedMutation || isRecordingPlaybackSessionCreate;

        if (isSafePost)
        {
            if (!gate.context.authenticated()) return rejectAuthentication(gate);
            gate.allowed = true;
            return gate;
        }

        if (isPost && gate.browserAuthenticated && !isExplicitlyAuthorizedPost)
        {
            AuthorizationDecision decision;
            decision.reasonCode = "security_policy_not_migrated";
            decision.permission = "unmapped.browser.mutation";
            decision.backendId = "*";
            decision.action = "http.browser.mutation";
            return rejectWithAudit(
                gate, decision, 503,
                "Browser-session mutations have not yet been migrated to the Phase 62 CSRF contract",
                "");
        }

        if (!isExplicitlyAuthorizedPost)
        {
            const bool explicitPolicyRequired = isPost &&
                (configuration_.mode == SecurityMode::Enforced ||
                 !usesLegacyCompatibilityCredential(gate.context));
            if (explicitPolicyRequired)
            {
                AuthorizationDecision decision;
                decision.reasonCode = "security_policy_not_migrated";
                decision.permission = "unmapped.mutation";
                decision.backendId = "*";
                decision.action = "http.mutation";
                return rejectWithAudit(
                    gate, decision, 503,
                    "This mutation route has not yet been migrated to the Phase 62 security contract",
                    "");
            }
            gate.allowed = true;
            return gate;
        }

        gate.protectedMutation = isProtectedMutation;
        AuthorizationRequest requestToAuthorize;
        requestToAuthorize.backendId = jsonStringValue(request.body, "backendId");
        bool recordingActionSupported = true;

        if (isMediaSessionMutation)
        {
            const std::string resourceKind = jsonStringValue(request.body, "resourceKind");
            if (resourceKind == "live-channel")
            {
                requestToAuthorize.permission = "media.live.play";
                requestToAuthorize.action = "media.live.play";
            }
            else
            {
                requestToAuthorize.permission = "media.recording.play";
                requestToAuthorize.action = "media.recording.play";
            }
        }
        else if (isRemoteAction)
        {
            requestToAuthorize.permission = "remote.control";
            requestToAuthorize.action = "remote.control";
        }
        else if (isTimerCreateAction)
        {
            requestToAuthorize.permission = "timers.create";
            requestToAuthorize.action = "timers.create";
        }
        else if (isTimerUpdateAction)
        {
            requestToAuthorize.permission = "timers.modify";
            requestToAuthorize.action = "timers.modify";
        }
        else if (isTimerDeleteAction)
        {
            requestToAuthorize.permission = "timers.delete";
            requestToAuthorize.action = "timers.delete";
        }
        else if (isChannelMoveAction)
        {
            requestToAuthorize.permission = "channels.move";
            requestToAuthorize.action = "channels.move";
        }
        else if (isSearchTimerCreateAction)
        {
            requestToAuthorize.permission = "searchtimers.create";
            requestToAuthorize.action = "searchtimers.create";
            defaultBackend(requestToAuthorize);
        }
        else if (isSearchTimerUpdateAction)
        {
            requestToAuthorize.permission = "searchtimers.modify";
            requestToAuthorize.action = "searchtimers.modify";
            defaultBackend(requestToAuthorize);
        }
        else if (isSearchTimerDeleteAction)
        {
            requestToAuthorize.permission = "searchtimers.delete";
            requestToAuthorize.action = "searchtimers.delete";
            defaultBackend(requestToAuthorize);
        }
        else if (isSearchTimerExecuteAction || isSearchTimerRealTestAction)
        {
            requestToAuthorize.permission = "searchtimers.execute";
            requestToAuthorize.action = "searchtimers.execute";
            defaultBackend(requestToAuthorize);
        }
        else if (isSearchTimerPreviewCacheRefreshAction)
        {
            requestToAuthorize.permission = "searchtimers.preview-cache.refresh";
            requestToAuthorize.action = "searchtimers.preview-cache.refresh";
            requestToAuthorize.backendId = queryStringValue(request.path, "backend");
            defaultBackend(requestToAuthorize);
        }
        else if (isEpgCacheRefreshAction)
        {
            requestToAuthorize.permission = "epg.cache.refresh";
            requestToAuthorize.action = "epg.cache.refresh";
            requestToAuthorize.backendId = queryStringValue(request.path, "backend");
            defaultBackend(requestToAuthorize);
        }
        else if (isNativeFuzzyStaleProbeDeleteAction)
        {
            requestToAuthorize.permission = "epgsearch.native-fuzzy.stale-probes.delete";
            requestToAuthorize.action = "epgsearch.native-fuzzy.stale-probes.delete";
            requestToAuthorize.backendId = "*";
        }
        else if (isNativeFuzzyRefreshAction)
        {
            requestToAuthorize.permission = "epgsearch.native-fuzzy.refresh";
            requestToAuthorize.action = "epgsearch.native-fuzzy.refresh";
            defaultBackend(requestToAuthorize);
        }
        else if (isManualRecordingMetadataAction)
        {
            requestToAuthorize.permission = "metadata.recording.assign";
            requestToAuthorize.action = "metadata.recording." + manualMetadataOperation;
            requestToAuthorize.backendId = manualMetadataBackendId;
        }
        else if (isSeriesArtworkSettingsAction)
        {
            requestToAuthorize.permission = "backend.settings.series-artwork.modify";
            requestToAuthorize.action = "backend.settings.series-artwork.modify";
        }
        else
        {
            const std::string recordingAction = jsonStringValue(request.body, "action");
            if (recordingAction == "RENAME")
            {
                requestToAuthorize.permission = "recordings.rename";
                requestToAuthorize.action = "recordings.rename";
            }
            else if (recordingAction == "MOVE")
            {
                requestToAuthorize.permission = "recordings.move";
                requestToAuthorize.action = "recordings.move";
            }
            else if (recordingAction == "DELETE")
            {
                requestToAuthorize.permission = "recordings.delete";
                requestToAuthorize.action = "recordings.delete";
            }
            else
            {
                recordingActionSupported = false;
                requestToAuthorize.permission = "recordings.execute";
                requestToAuthorize.action = "recordings.execute";
            }
        }

        const std::string operationId = jsonStringValue(request.body, "operationId");

        if (gate.browserAuthenticated &&
            (browserSessionAuthenticator_ == nullptr ||
             !browserSessionAuthenticator_->verifyCsrf(request.headers)))
        {
            AuthorizationDecision decision;
            decision.reasonCode = "csrf_validation_failed";
            decision.permission = requestToAuthorize.permission;
            decision.backendId = requestToAuthorize.backendId;
            decision.action = requestToAuthorize.action;
            return rejectWithAudit(
                gate, decision, 403, "A valid CSRF token is required", operationId);
        }

        if (isRecordingExecutionAction && !recordingActionSupported)
        {
            AuthorizationDecision decision;
            decision.reasonCode = "invalid_recording_action";
            decision.permission = requestToAuthorize.permission;
            decision.backendId = requestToAuthorize.backendId;
            decision.action = requestToAuthorize.action;
            return rejectWithAudit(
                gate, decision, 400, "Unsupported recording execution action", operationId);
        }

        const AuthorizationDecision decision =
            authorizationService_.authorize(gate.context, requestToAuthorize);

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
            const int statusCode =
                decision.reasonCode == "invalid_backend_scope"
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

        gate.authorizationDecision = decision;
        gate.operationId = operationId;
        gate.allowed = true;
        return gate;
    }

    void decorateResponse(
        const RequestSecurityContext& context,
        HttpServerResponse& response) const
    {
        response.headers["X-Request-ID"] = context.requestId;
        if (!context.correlationId.empty())
            response.headers["X-Correlation-ID"] = context.correlationId;
    }

    bool appendProtectedMutationOutcome(
        const SecurityGateDecision& gate,
        int statusCode) const
    {
        if (!gate.allowed || !gate.protectedMutation ||
            !gate.authorizationDecision.allowed) return false;

        const bool succeeded = statusCode >= 200 && statusCode <= 299;
        AccountabilityEvent event;
        event.eventId = opaqueId("audit");
        event.classes = succeeded ? "audit" : "audit,security";
        event.eventType = succeeded ? "operation.succeeded" : "operation.failed";
        event.severity = succeeded ? "info" : "error";
        event.occurredAt = nowUtc();
        event.actorId = gate.context.actor.actorId.empty() ? "anonymous" : gate.context.actor.actorId;
        event.actorType = actorTypeName(gate.context.actor.type);
        event.deviceId = gate.context.device ? gate.context.device->deviceId : "";
        event.sessionId = gate.context.session ? gate.context.session->sessionId : "";
        event.authenticationState = authenticationStateName(gate.context.authenticationState);
        event.permission = gate.authorizationDecision.permission;
        event.backendId = gate.authorizationDecision.backendId;
        event.operationId = gate.operationId;
        event.requestId = gate.context.requestId;
        event.correlationId = gate.context.correlationId;
        event.action = gate.authorizationDecision.action;
        event.decision = "allowed";
        event.reasonCode = "http_status_" + std::to_string(statusCode);
        event.outcome = succeeded ? "succeeded" : "failed";
        return accountabilityRepository_.append(event);
    }

    HttpServerResponse outcomeAccountabilityUnavailableResponse(
        const RequestSecurityContext& context) const
    {
        return errorResponse(
            503,
            "accountability_unavailable",
            "Security outcome accountability persistence is unavailable",
            context);
    }

private:
    static void defaultBackend(AuthorizationRequest& request)
    {
        if (request.backendId.empty()) request.backendId = "default";
    }

    static std::string lowerAscii(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char character) {
                return static_cast<char>(std::tolower(character));
            });
        return value;
    }

    static std::string requestPath(const std::string& target)
    {
        const std::size_t query = target.find('?');
        return query == std::string::npos ? target : target.substr(0, query);
    }

    static bool manualRecordingMetadataRoute(
        const std::string& path,
        std::string& backendId,
        std::string& operation)
    {
        backendId.clear();
        operation.clear();
        const std::string prefix = "/api/backends/";
        const std::string segment = "/recordings/metadata/";
        if (path.compare(0, prefix.size(), prefix) != 0) return false;
        const std::size_t separator = path.find(segment, prefix.size());
        if (separator == std::string::npos) return false;
        backendId = path.substr(prefix.size(), separator - prefix.size());
        operation = path.substr(separator + segment.size());
        const bool validBackend =
            !backendId.empty() && backendId.size() <= 128U &&
            std::all_of(backendId.begin(), backendId.end(),
                [](unsigned char character) {
                    return std::isalnum(character) || character == '.' ||
                        character == '_' || character == '-';
                });
        if (!validBackend || operation.empty() || operation.find('/') != std::string::npos)
            return false;
        return operation == "search" || operation == "seasons" ||
            operation == "episodes" || operation == "assign" || operation == "withdraw";
    }

    static int hexValue(char value)
    {
        if (value >= '0' && value <= '9') return value - '0';
        if (value >= 'A' && value <= 'F') return value - 'A' + 10;
        if (value >= 'a' && value <= 'f') return value - 'a' + 10;
        return -1;
    }

    static std::string urlDecode(const std::string& value)
    {
        std::string decoded;
        decoded.reserve(value.size());
        for (std::size_t index = 0; index < value.size(); ++index)
        {
            const char current = value[index];
            if (current == '+')
            {
                decoded.push_back(' ');
                continue;
            }
            if (current == '%' && index + 2 < value.size())
            {
                const int high = hexValue(value[index + 1]);
                const int low = hexValue(value[index + 2]);
                if (high >= 0 && low >= 0)
                {
                    decoded.push_back(static_cast<char>((high << 4) | low));
                    index += 2;
                    continue;
                }
            }
            decoded.push_back(current);
        }
        return decoded;
    }

    static std::string queryStringValue(
        const std::string& target,
        const std::string& key)
    {
        const std::size_t queryStart = target.find('?');
        if (queryStart == std::string::npos || queryStart + 1 >= target.size()) return "";
        std::string result;
        std::size_t position = queryStart + 1;
        while (position <= target.size())
        {
            const std::size_t separator = target.find('&', position);
            const std::string item = target.substr(
                position,
                separator == std::string::npos ? std::string::npos : separator - position);
            const std::size_t equals = item.find('=');
            const std::string itemKey = urlDecode(
                equals == std::string::npos ? item : item.substr(0, equals));
            if (itemKey == key)
                result = equals == std::string::npos ? "" : urlDecode(item.substr(equals + 1));
            if (separator == std::string::npos) break;
            position = separator + 1;
        }
        return result;
    }

    static std::string headerValue(
        const HttpServerRequest& request,
        const std::string& name)
    {
        const std::string wanted = lowerAscii(name);
        for (const auto& header : request.headers)
            if (lowerAscii(header.first) == wanted) return header.second;
        return "";
    }

    static std::string jsonStringValue(
        const std::string& body,
        const std::string& field)
    {
        const std::string needle = "\"" + field + "\"";
        std::size_t position = body.find(needle);
        if (position == std::string::npos) return "";
        position = body.find(':', position + needle.size());
        if (position == std::string::npos) return "";
        do { ++position; }
        while (position < body.size() && std::isspace(static_cast<unsigned char>(body[position])));
        if (position >= body.size() || body[position] != '"') return "";

        ++position;
        std::string value;
        while (position < body.size())
        {
            const char character = body[position++];
            if (character == '"') return value;
            if (character != '\\')
            {
                value.push_back(character);
                continue;
            }
            if (position >= body.size()) return "";
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
                        escaped.push_back(character);
                    break;
            }
        }
        return escaped;
    }

    static bool safeContextToken(const std::string& value)
    {
        if (value.empty() || value.size() > 128) return false;
        return std::all_of(value.begin(), value.end(),
            [](unsigned char character) {
                return std::isalnum(character) || character == '-' ||
                    character == '_' || character == '.' || character == ':';
            });
    }

    static std::string authenticationReason(const RequestSecurityContext& context)
    {
        if (context.authenticationState == AuthenticationState::Anonymous) return "authentication_required";
        if (context.authenticationState == AuthenticationState::Invalid) return "invalid_credentials";
        if (!context.actor.active || context.actor.actorId.empty()) return "actor_revoked";
        if (context.device.has_value() && !context.device->active) return "device_revoked";
        if (context.credential.has_value())
        {
            if (context.credential->revoked || !context.credential->active) return "credential_revoked";
            if (context.credential->expired) return "credential_expired";
        }
        if (context.session.has_value())
        {
            if (context.session->revoked || !context.session->active) return "session_revoked";
            if (context.session->expired) return "session_expired";
        }
        if (context.authenticationState == AuthenticationState::Expired) return "session_expired";
        if (context.authenticationState == AuthenticationState::Revoked) return "session_revoked";
        return "invalid_credentials";
    }

    static bool authenticationFailure(const AuthorizationDecision& decision)
    {
        return decision.reasonCode == "authentication_required" ||
            decision.reasonCode == "invalid_credentials" ||
            decision.reasonCode == "session_expired" ||
            decision.reasonCode == "session_revoked" ||
            decision.reasonCode == "credential_expired" ||
            decision.reasonCode == "credential_revoked" ||
            decision.reasonCode == "actor_revoked" ||
            decision.reasonCode == "device_revoked";
    }

    static std::string messageForReason(const std::string& reasonCode)
    {
        if (reasonCode == "authentication_required") return "Authentication is required";
        if (reasonCode == "invalid_credentials") return "The supplied credentials are invalid";
        if (reasonCode == "session_expired") return "The authenticated session has expired";
        if (reasonCode == "credential_expired") return "The authenticated credential has expired";
        if (reasonCode == "session_revoked" || reasonCode == "credential_revoked" ||
            reasonCode == "actor_revoked" || reasonCode == "device_revoked")
            return "The authenticated identity is no longer active";
        if (reasonCode == "backend_scope_denied") return "The actor is not permitted to mutate this backend";
        if (reasonCode == "permission_denied") return "The actor lacks the required permission";
        if (reasonCode == "permission_grants_unavailable") return "Browser permission persistence is unavailable";
        if (reasonCode == "invalid_backend_scope") return "A valid backend scope is required";
        if (reasonCode == "csrf_validation_failed") return "A valid CSRF token is required";
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
        const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
        const unsigned long long sequence = idCounter_.fetch_add(1) + 1;
        std::ostringstream output;
        output << prefix << '-' << std::hex << static_cast<unsigned long long>(ticks)
               << '-' << sequence;
        return output.str();
    }

    bool usesLegacyCompatibilityCredential(const RequestSecurityContext& context) const
    {
        return context.authenticated() && context.actor.actorId == configuration_.actorId &&
            context.credential.has_value() &&
            context.credential->credentialId == configuration_.credentialId;
    }

    RequestSecurityContext resolvePersistentIdentity(RequestSecurityContext context) const
    {
        if (persistentIdentityResolver_ != nullptr)
            context = persistentIdentityResolver_->resolve(std::move(context));
        return context;
    }

    AuthenticationResult authenticate(const HttpServerRequest& request) const
    {
        AuthenticationResult result;
        std::string requestId = headerValue(request, "X-Request-ID");
        if (!safeContextToken(requestId)) requestId = opaqueId("req");

        std::string correlationId = headerValue(request, "X-Correlation-ID");
        if (!correlationId.empty() && !safeContextToken(correlationId)) correlationId.clear();

        if (browserSessionAuthenticator_ != nullptr &&
            browserSessionAuthenticator_->hasSessionCookie(request.headers))
        {
            result.browserSessionPresented = true;
            result.context = browserSessionAuthenticator_->authenticate(
                request.headers, requestId, correlationId);
            if (result.context.authenticated())
            {
                result.context = resolvePersistentIdentity(std::move(result.context));
                result.browserAuthenticated = result.context.authenticated();
            }
            return result;
        }

        result.context = legacyAuthenticator_.authenticate(
            request.headers, requestId, correlationId);
        if (result.context.authenticated())
        {
            result.context = resolvePersistentIdentity(std::move(result.context));
            return result;
        }

        if (managedBasicAuthenticator_ != nullptr)
        {
            RequestSecurityContext managedContext = managedBasicAuthenticator_->authenticate(
                request.headers, requestId, correlationId);
            if (managedContext.authenticated())
            {
                result.context = resolvePersistentIdentity(std::move(managedContext));
                return result;
            }
        }
        return result;
    }

    SecurityGateDecision rejectAuthentication(SecurityGateDecision gate) const
    {
        AuthorizationDecision decision;
        decision.reasonCode = authenticationReason(gate.context);
        decision.action = "http.access";
        decision.permission = "legacy.compatibility.access";
        decision.backendId = "*";
        return rejectWithAudit(
            gate, decision, 401, messageForReason(decision.reasonCode), "");
    }

    bool appendDecisionEvent(
        const RequestSecurityContext& context,
        const AuthorizationDecision& decision,
        const std::string& operationId) const
    {
        AccountabilityEvent event;
        event.eventId = opaqueId("audit");
        event.classes = decision.allowed ? "audit" : "audit,security";
        event.eventType = decision.allowed ? "authorization.allowed" : "authorization.denied";
        event.severity = decision.allowed ? "info" : "warning";
        event.occurredAt = nowUtc();
        event.actorId = context.actor.actorId.empty() ? "anonymous" : context.actor.actorId;
        event.actorType = actorTypeName(context.actor.type);
        event.deviceId = context.device ? context.device->deviceId : "";
        event.sessionId = context.session ? context.session->sessionId : "";
        event.authenticationState = authenticationStateName(context.authenticationState);
        event.permission = decision.permission;
        event.backendId = decision.backendId;
        event.operationId = operationId;
        event.requestId = context.requestId;
        event.correlationId = context.correlationId;
        event.action = decision.action;
        event.decision = decision.allowed ? "allowed" : "denied";
        event.reasonCode = decision.reasonCode;
        event.outcome = decision.allowed ? "dispatch_authorized" : "dispatch_denied";
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
            statusCode, decision.reasonCode, message, gate.context, advertiseBasic);
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
            response.headers["WWW-Authenticate"] = "Basic realm=\"VDR-Suite\", charset=\"UTF-8\"";
        response.body =
            "{\"error\":{\"code\":\"" + jsonEscape(code) +
            "\",\"message\":\"" + jsonEscape(message) +
            "\",\"requestId\":\"" + jsonEscape(context.requestId) + "\"}}";
        decorateResponse(context, response);
        return response;
    }

    SecurityConfiguration configuration_;
    LegacyBasicAuthenticator legacyAuthenticator_;
    AuthorizationService authorizationService_;
    AccountabilityEventRepository& accountabilityRepository_;
    const PersistentIdentityResolver* persistentIdentityResolver_;
    const ManagedBasicAuthenticator* managedBasicAuthenticator_;
    const BrowserSessionAuthenticator* browserSessionAuthenticator_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};