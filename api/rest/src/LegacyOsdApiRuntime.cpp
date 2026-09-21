#include "LegacyOsdApiRuntime.h"

#include "LegacyOsdSessionService.h"
#include "OsdViewerBindingService.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
constexpr const char* SessionsRoute = "/api/vdr/legacy-osd/sessions";
constexpr const char* SessionStatusRoute =
    "/api/vdr/legacy-osd/sessions/status";
constexpr const char* ViewersRoute =
    "/api/vdr/legacy-osd/viewers";
constexpr const char* ViewerDetachRoute =
    "/api/vdr/legacy-osd/viewers/detach";
constexpr std::size_t MaximumBodyBytes = 1024U;

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
}

bool safeToken(const std::string& value, bool allowColon,
               std::size_t maximumBytes = 128U)
{
    return !value.empty() && value.size() <= maximumBytes &&
        std::all_of(value.begin(), value.end(),
            [allowColon](unsigned char c) {
                return std::isalnum(c) != 0 || c == '-' || c == '_' ||
                    c == '.' || (allowColon && c == ':');
            });
}

void skipWhitespace(const std::string& value, std::size_t& position)
{
    while (position < value.size() &&
           std::isspace(static_cast<unsigned char>(value[position])) != 0)
        ++position;
}

bool consume(const std::string& value, std::size_t& position, char expected)
{
    skipWhitespace(value, position);
    if (position >= value.size() || value[position] != expected) return false;
    ++position;
    return true;
}

bool parseString(const std::string& value, std::size_t& position,
                 std::string& output)
{
    skipWhitespace(value, position);
    if (position >= value.size() || value[position] != '"') return false;
    ++position;
    output.clear();
    while (position < value.size())
    {
        const unsigned char c = static_cast<unsigned char>(value[position++]);
        if (c == '"') return true;
        if (c < 0x20U || c == '\\') return false;
        output.push_back(static_cast<char>(c));
        if (output.size() > 128U) return false;
    }
    return false;
}

bool parseCreateBody(const std::string& body, std::string& backendId)
{
    backendId.clear();
    if (body.empty() || body.size() > MaximumBodyBytes) return false;
    std::size_t position = 0;
    std::string key;
    if (!consume(body, position, '{') ||
        !parseString(body, position, key) || key != "backendId" ||
        !consume(body, position, ':') ||
        !parseString(body, position, backendId) ||
        !consume(body, position, '}'))
        return false;
    skipWhitespace(body, position);
    return position == body.size() && safeToken(backendId, false);
}

bool parseStringObject(
    const std::string& body,
    const std::vector<std::string>& expectedKeys,
    std::map<std::string, std::string>& fields)
{
    fields.clear();
    if (body.empty() || body.size() > MaximumBodyBytes) return false;
    std::size_t position = 0;
    if (!consume(body, position, '{')) return false;

    while (true)
    {
        skipWhitespace(body, position);
        if (position < body.size() && body[position] == '}')
        {
            ++position;
            break;
        }

        std::string key;
        std::string value;
        if (!parseString(body, position, key) ||
            std::find(expectedKeys.begin(), expectedKeys.end(), key) ==
                expectedKeys.end() ||
            fields.count(key) != 0U ||
            !consume(body, position, ':') ||
            !parseString(body, position, value) ||
            !safeToken(value, false))
            return false;
        fields.emplace(key, value);

        skipWhitespace(body, position);
        if (position < body.size() && body[position] == '}')
        {
            ++position;
            break;
        }
        if (!consume(body, position, ',')) return false;
    }

    skipWhitespace(body, position);
    if (position != body.size() ||
        fields.size() != expectedKeys.size())
        return false;
    for (const std::string& key : expectedKeys)
    {
        if (fields.count(key) == 0U) return false;
    }
    return true;
}

bool parseViewerBody(
    const std::string& body,
    bool detach,
    std::string& backendId,
    std::string& sessionId,
    std::string& viewerId)
{
    std::map<std::string, std::string> fields;
    std::vector<std::string> keys = {
        "backendId", "legacyOsdSessionId"
    };
    if (detach) keys.push_back("viewerBindingId");
    if (!parseStringObject(body, keys, fields)) return false;
    backendId = fields["backendId"];
    sessionId = fields["legacyOsdSessionId"];
    viewerId = detach ? fields["viewerBindingId"] : std::string{};
    return true;
}

bool parseStatusTarget(const std::string& target, std::string& backendId,
                       std::string& sessionId)
{
    backendId.clear();
    sessionId.clear();
    if (requestPath(target) != SessionStatusRoute) return false;
    const std::size_t query = target.find('?');
    if (query == std::string::npos || query + 1U >= target.size()) return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = query + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start, end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;
        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        const std::string value = item.substr(equals + 1U);
        if ((key != "backend" && key != "session") ||
            parameters.count(key) != 0U || !safeToken(value, false))
            return false;
        parameters.emplace(key, value);
        if (end == std::string::npos) break;
        start = end + 1U;
    }
    if (parameters.size() != 2U) return false;
    backendId = parameters["backend"];
    sessionId = parameters["session"];
    return safeToken(backendId, false) && safeToken(sessionId, false);
}

std::string jsonEscape(const std::string& value)
{
    std::string output;
    for (unsigned char c : value)
    {
        if (c == '"' || c == '\\')
        {
            output.push_back('\\');
            output.push_back(static_cast<char>(c));
        }
        else if (c >= 0x20U) output.push_back(static_cast<char>(c));
    }
    return output;
}

ApiResponse jsonResponse(int statusCode, const std::string& body)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = body;
    return response;
}

ApiResponse errorResponse(int statusCode, const std::string& error)
{
    return jsonResponse(statusCode,
        "{\"error\":{\"code\":\"" + jsonEscape(error) +
        "\",\"message\":\"Legacy OSD session request rejected\"}}");
}

std::string viewerJson(const OsdViewerBinding& binding)
{
    std::ostringstream output;
    output << "{\"viewerBindingId\":\""
           << jsonEscape(binding.viewerBindingId)
           << "\",\"bindingRevision\":" << binding.bindingRevision
           << ",\"legacyOsdSessionId\":\""
           << jsonEscape(binding.legacyOsdSessionId)
           << "\",\"sessionRevision\":" << binding.sessionRevision
           << ",\"backendId\":\"" << jsonEscape(binding.backendId)
           << "\",\"backendGeneration\":" << binding.backendGeneration
           << ",\"state\":\"" << osdViewerBindingStateName(binding.state)
           << "\",\"attachedAt\":" << binding.attachedAt
           << ",\"lastSeenAt\":" << binding.lastSeenAt
           << ",\"expiresAt\":" << binding.expiresAt
           << ",\"renderingProfile\":\""
           << jsonEscape(binding.renderingProfile)
           << "\",\"surface\":{\"surfaceId\":\""
           << jsonEscape(binding.osdSurfaceId)
           << "\",\"osdEpoch\":\"" << jsonEscape(binding.osdEpoch)
           << "\"},\"cursor\":{\"acknowledgedFrameSequence\":"
           << binding.lastAcknowledgedFrameSequence
           << ",\"deliveredFrameSequence\":"
           << binding.lastDeliveredFrameSequence
           << ",\"acknowledgedEventSequence\":"
           << binding.lastAcknowledgedEventSequence
           << "},\"capabilities\":{\"view\":true,\"control\":false}"
           << ",\"closeReason\":\"" << jsonEscape(binding.closeReason)
           << "\"}";
    return output.str();
}

std::string sessionJson(const LegacyOsdSession& session)
{
    std::ostringstream output;
    output << "{\"legacyOsdSessionId\":\""
           << jsonEscape(session.legacyOsdSessionId)
           << "\",\"sessionRevision\":" << session.sessionRevision
           << ",\"backendId\":\"" << jsonEscape(session.backendId)
           << "\",\"backendGeneration\":" << session.backendGeneration
           << ",\"mode\":\"view_only\""
           << ",\"state\":\"" << legacyOsdSessionStateName(session.state)
           << "\",\"createdAt\":" << session.createdAt
           << ",\"expiresAt\":" << session.expiresAt
           << ",\"lastActivityAt\":" << session.lastActivityAt
           << ",\"capabilities\":{\"view\":"
           << (session.capabilitySnapshot.viewAvailable ? "true" : "false")
           << ",\"control\":"
           << (session.capabilitySnapshot.controlAvailable ? "true" : "false")
           << "},\"policy\":{\"view\":"
           << (session.policySnapshot.viewAuthorized ? "true" : "false")
           << ",\"control\":"
           << (session.policySnapshot.controlAuthorized ? "true" : "false")
           << "},\"surface\":{\"surfaceId\":\""
           << jsonEscape(session.osdSurfaceId)
           << "\",\"osdEpoch\":\"" << jsonEscape(session.osdEpoch)
           << "\"},\"closeReason\":\"" << jsonEscape(session.closeReason)
           << "\"}";
    return output.str();
}

int errorStatus(const std::string& error)
{
    if (error == "legacy_osd_session_request_invalid") return 400;
    if (error == "legacy_osd_view_not_authorized") return 403;
    if (error == "legacy_osd_session_not_found") return 404;
    if (error == "legacy_osd_session_expired") return 410;
    if (error == "legacy_osd_backend_generation_changed") return 409;
    if (error == "legacy_osd_session_capacity_reached") return 429;
    if (error == "legacy_osd_viewer_request_invalid") return 400;
    if (error == "legacy_osd_viewer_not_found") return 404;
    if (error == "legacy_osd_viewer_expired") return 410;
    if (error == "legacy_osd_viewer_capacity_reached" ||
        error == "legacy_osd_viewer_session_capacity_reached") return 429;
    if (error == "legacy_osd_viewer_concurrent_update") return 409;
    return 503;
}
}

LegacyOsdApiRuntime& LegacyOsdApiRuntime::instance()
{
    static LegacyOsdApiRuntime runtime;
    return runtime;
}

bool LegacyOsdApiRuntime::configure(LegacyOsdSessionService& service)
{
    sessionService_ = &service;
    viewerService_ = nullptr;
    return true;
}

bool LegacyOsdApiRuntime::configure(
    LegacyOsdSessionService& sessionService,
    OsdViewerBindingService& viewerService)
{
    sessionService_ = &sessionService;
    viewerService_ = &viewerService;
    return true;
}

void LegacyOsdApiRuntime::reset()
{
    viewerService_ = nullptr;
    sessionService_ = nullptr;
}

bool LegacyOsdApiRuntime::configured() const
{
    return sessionService_ != nullptr;
}

bool LegacyOsdApiRuntime::tryHandlePost(
    const std::string& requestTarget, const std::string& body,
    const std::string& actorRef, const std::string& clientRef,
    const std::string& correlationRef, ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);
    if (path != SessionsRoute &&
        path != ViewersRoute &&
        path != ViewerDetachRoute)
        return false;

    if (path == ViewersRoute || path == ViewerDetachRoute)
    {
        if (!viewerService_)
        {
            response = errorResponse(
                503, "legacy_osd_viewer_runtime_unavailable");
            return true;
        }

        std::string backendId;
        std::string sessionId;
        std::string viewerId;
        const bool detach = path == ViewerDetachRoute;
        if (!parseViewerBody(
                body, detach, backendId, sessionId, viewerId) ||
            !safeToken(actorRef, true) || !safeToken(clientRef, true))
        {
            response = errorResponse(
                400, "legacy_osd_viewer_request_invalid");
            return true;
        }

        if (detach)
        {
            OsdViewerDetachRequest request;
            request.actorId = actorRef;
            request.clientInstanceId = clientRef;
            request.backendId = backendId;
            request.legacyOsdSessionId = sessionId;
            request.viewerBindingId = viewerId;
            const auto result = viewerService_->detach(request);
            if (!result.accepted)
            {
                response = errorResponse(
                    errorStatus(result.error), result.error);
                return true;
            }
            response = jsonResponse(200, viewerJson(result.binding));
            return true;
        }

        OsdViewerAttachRequest request;
        request.actorId = actorRef;
        request.clientInstanceId = clientRef;
        request.backendId = backendId;
        request.legacyOsdSessionId = sessionId;
        const auto result = viewerService_->attach(request);
        if (!result.accepted)
        {
            response = errorResponse(
                errorStatus(result.error), result.error);
            return true;
        }
        response = jsonResponse(201, viewerJson(result.binding));
        return true;
    }

    if (!sessionService_)
    {
        response = errorResponse(503, "legacy_osd_session_runtime_unavailable");
        return true;
    }

    std::string backendId;
    if (!parseCreateBody(body, backendId) ||
        !safeToken(actorRef, true) || !safeToken(clientRef, true) ||
        !safeToken(correlationRef, true))
    {
        response = errorResponse(400, "legacy_osd_session_request_invalid");
        return true;
    }

    LegacyOsdSessionCreateRequest request;
    request.actorId = actorRef;
    request.clientInstanceId = clientRef;
    request.backendId = backendId;
    request.correlationId = correlationRef;

    const auto result = sessionService_->create(request);
    if (!result.accepted)
    {
        response = errorResponse(errorStatus(result.error), result.error);
        return true;
    }
    response = jsonResponse(201, sessionJson(result.session));
    return true;
}

bool LegacyOsdApiRuntime::tryHandleGet(
    const std::string& requestTarget, ApiResponse& response,
    const std::string& actorRef, const std::string& clientRef) const
{
    if (requestPath(requestTarget) != SessionStatusRoute) return false;
    if (!sessionService_)
    {
        response = errorResponse(503, "legacy_osd_session_runtime_unavailable");
        return true;
    }

    std::string backendId;
    std::string sessionId;
    if (!parseStatusTarget(requestTarget, backendId, sessionId) ||
        !safeToken(actorRef, true) || !safeToken(clientRef, true))
    {
        response = errorResponse(400, "legacy_osd_session_request_invalid");
        return true;
    }

    const auto result =
        sessionService_->status(sessionId, actorRef, clientRef, backendId);
    if (!result.accepted)
    {
        response = errorResponse(errorStatus(result.error), result.error);
        return true;
    }
    response = jsonResponse(200, sessionJson(result.session));
    return true;
}
