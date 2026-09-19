#include "HbbtvApiRuntime.h"

#include "HbbtvApplicationSessionService.h"
#include "HbbtvControlPlaneReadService.h"
#include "SuiteBridgeHbbtvPresentationResolver.h"
#include "SuiteBridgeHbbtvMediaResolver.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace
{

constexpr const char* ApplicationsRoute =
    "/api/vdr/broadcast/hbbtv/applications";
constexpr const char* SessionsRoute =
    "/api/vdr/broadcast/hbbtv/sessions";
constexpr const char* SessionStatusRoute =
    "/api/vdr/broadcast/hbbtv/sessions/status";
constexpr const char* SessionInputRoute =
    "/api/vdr/broadcast/hbbtv/sessions/input";
constexpr const char* SessionCloseRoute =
    "/api/vdr/broadcast/hbbtv/sessions/close";
constexpr const char* SessionPresentationRoute =
    "/api/vdr/broadcast/hbbtv/sessions/presentation";
constexpr const char* SessionMediaRoute =
    "/api/vdr/broadcast/hbbtv/sessions/media";

constexpr std::size_t MaximumBackendIdBytes = 128U;
constexpr std::size_t MaximumChannelIdBytes = 63U;
constexpr std::size_t MaximumIdentityBytes = 128U;
constexpr std::size_t MaximumBodyBytes = 4096U;

std::string requestPath(const std::string& target)
{
    const std::size_t query = target.find('?');
    return query == std::string::npos ? target : target.substr(0, query);
}

int hexValue(char character)
{
    if (character >= '0' && character <= '9') return character - '0';
    if (character >= 'a' && character <= 'f') return character - 'a' + 10;
    if (character >= 'A' && character <= 'F') return character - 'A' + 10;
    return -1;
}

bool urlDecode(
    const std::string& input,
    std::string& output,
    std::size_t maximumBytes)
{
    output.clear();
    if (input.size() > maximumBytes * 3U) return false;

    for (std::size_t index = 0; index < input.size(); ++index)
    {
        const unsigned char character =
            static_cast<unsigned char>(input[index]);
        if (character == '+')
        {
            output.push_back(' ');
        }
        else if (character == '%')
        {
            if (index + 2U >= input.size()) return false;
            const int high = hexValue(input[index + 1U]);
            const int low = hexValue(input[index + 2U]);
            if (high < 0 || low < 0) return false;
            const unsigned char decoded =
                static_cast<unsigned char>((high << 4) | low);
            if (decoded < 0x20U || decoded == 0x7fU) return false;
            output.push_back(static_cast<char>(decoded));
            index += 2U;
        }
        else
        {
            if (character < 0x20U || character == 0x7fU) return false;
            output.push_back(static_cast<char>(character));
        }

        if (output.size() > maximumBytes) return false;
    }

    return true;
}

bool safeToken(
    const std::string& value,
    std::size_t maximumBytes,
    bool allowColon)
{
    return !value.empty() && value.size() <= maximumBytes &&
        std::all_of(
            value.begin(),
            value.end(),
            [allowColon](unsigned char character) {
                return std::isalnum(character) != 0 ||
                    character == '-' ||
                    character == '_' ||
                    character == '.' ||
                    (allowColon && character == ':');
            });
}

bool validBackendId(const std::string& value)
{
    return safeToken(value, MaximumBackendIdBytes, false);
}

bool validChannelId(const std::string& value)
{
    return safeToken(value, MaximumChannelIdBytes, true);
}

bool validIdentity(const std::string& value)
{
    return safeToken(value, MaximumIdentityBytes, true);
}

struct HbbtvRequest
{
    std::string backendId;
    std::string channelId;
};

bool parseRequest(const std::string& target, HbbtvRequest& request)
{
    request = {};
    if (requestPath(target) != ApplicationsRoute) return false;

    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos || queryStart + 1U >= target.size())
        return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if ((key != "backend" && key != "channel") ||
            parameters.count(key) != 0U)
        {
            return false;
        }

        std::string decoded;
        const std::size_t maximum =
            key == "backend" ? MaximumBackendIdBytes : MaximumChannelIdBytes;
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    if (parameters.size() != 2U) return false;
    const auto backend = parameters.find("backend");
    const auto channel = parameters.find("channel");
    if (backend == parameters.end() || channel == parameters.end())
        return false;

    request.backendId = backend->second;
    request.channelId = channel->second;
    return validBackendId(request.backendId) &&
        validChannelId(request.channelId);
}


struct HbbtvPresentationRequest
{
    std::string backendId;
    std::string sessionId;
    std::uint64_t knownRevision = 0;
};

bool parsePresentationRequest(
    const std::string& target,
    HbbtvPresentationRequest& request)
{
    request = {};
    if (requestPath(target) != SessionPresentationRoute) return false;

    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos || queryStart + 1U >= target.size())
        return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;

        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if ((key != "backend" && key != "session" && key != "revision") ||
            parameters.count(key) != 0U)
            return false;

        std::string decoded;
        const std::size_t maximum =
            key == "backend" ? MaximumBackendIdBytes : MaximumIdentityBytes;
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    const auto backend = parameters.find("backend");
    const auto session = parameters.find("session");
    if (backend == parameters.end() || session == parameters.end())
        return false;

    request.backendId = backend->second;
    request.sessionId = session->second;
    if (!validBackendId(request.backendId) ||
        !validIdentity(request.sessionId))
        return false;

    const auto revision = parameters.find("revision");
    if (revision != parameters.end())
    {
        if (revision->second.empty() ||
            !std::all_of(
                revision->second.begin(),
                revision->second.end(),
                [](unsigned char character) {
                    return std::isdigit(character) != 0;
                }))
            return false;

        std::uint64_t value = 0;
        for (unsigned char character : revision->second)
        {
            const unsigned int digit =
                static_cast<unsigned int>(character - '0');
            if (value >
                (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
                return false;
            value = value * 10U + digit;
        }
        request.knownRevision = value;
    }

    return parameters.size() == (revision != parameters.end() ? 3U : 2U);
}

struct HbbtvMediaRequest
{
    std::string backendId;
    std::string sessionId;
};

bool parseMediaRequest(
    const std::string& target,
    HbbtvMediaRequest& request)
{
    request = {};
    if (requestPath(target) != SessionMediaRoute) return false;

    const std::size_t queryStart = target.find('?');
    if (queryStart == std::string::npos || queryStart + 1U >= target.size())
        return false;

    std::map<std::string, std::string> parameters;
    std::size_t start = queryStart + 1U;
    while (start <= target.size())
    {
        const std::size_t end = target.find('&', start);
        const std::string item = target.substr(
            start,
            end == std::string::npos ? std::string::npos : end - start);
        if (item.empty()) return false;
        const std::size_t equals = item.find('=');
        if (equals == std::string::npos || equals == 0U) return false;
        const std::string key = item.substr(0, equals);
        if ((key != "backend" && key != "session") ||
            parameters.count(key) != 0U)
            return false;

        std::string decoded;
        const std::size_t maximum =
            key == "backend" ? MaximumBackendIdBytes : MaximumIdentityBytes;
        if (!urlDecode(item.substr(equals + 1U), decoded, maximum))
            return false;
        parameters.emplace(key, std::move(decoded));

        if (end == std::string::npos) break;
        start = end + 1U;
    }

    if (parameters.size() != 2U) return false;
    const auto backend = parameters.find("backend");
    const auto session = parameters.find("session");
    if (backend == parameters.end() || session == parameters.end())
        return false;

    request.backendId = backend->second;
    request.sessionId = session->second;
    return validBackendId(request.backendId) &&
        validIdentity(request.sessionId);
}

void skipWhitespace(const std::string& body, std::size_t& position)
{
    while (position < body.size() &&
           std::isspace(static_cast<unsigned char>(body[position])))
    {
        ++position;
    }
}

bool parseJsonString(
    const std::string& body,
    std::size_t& position,
    std::string& value,
    std::size_t maximumBytes)
{
    skipWhitespace(body, position);
    if (position >= body.size() || body[position] != '"') return false;
    ++position;
    value.clear();

    while (position < body.size())
    {
        const unsigned char character =
            static_cast<unsigned char>(body[position++]);
        if (character == '"')
            return !value.empty() && value.size() <= maximumBytes;
        if (character < 0x20U) return false;

        if (character != '\\')
        {
            value.push_back(static_cast<char>(character));
        }
        else
        {
            if (position >= body.size()) return false;
            const char escaped = body[position++];
            switch (escaped)
            {
                case '"': value.push_back('"'); break;
                case '\\': value.push_back('\\'); break;
                case '/': value.push_back('/'); break;
                case 'b': value.push_back('\b'); break;
                case 'f': value.push_back('\f'); break;
                case 'n': value.push_back('\n'); break;
                case 'r': value.push_back('\r'); break;
                case 't': value.push_back('\t'); break;
                default: return false;
            }
        }

        if (value.size() > maximumBytes) return false;
    }

    return false;
}

bool parseJsonUnsigned(
    const std::string& body,
    std::size_t& position,
    std::uint64_t& value)
{
    skipWhitespace(body, position);
    if (position >= body.size() ||
        !std::isdigit(static_cast<unsigned char>(body[position])))
    {
        return false;
    }

    value = 0;
    while (position < body.size() &&
           std::isdigit(static_cast<unsigned char>(body[position])))
    {
        const unsigned int digit =
            static_cast<unsigned int>(body[position] - '0');
        if (value >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10U)
        {
            return false;
        }
        value = value * 10U + digit;
        ++position;
    }
    return true;
}

struct FlatJson
{
    std::map<std::string, std::string> strings;
    std::map<std::string, std::uint64_t> unsigneds;
};

bool parseFlatJson(const std::string& body, FlatJson& json)
{
    json = {};
    if (body.empty() || body.size() > MaximumBodyBytes) return false;

    std::size_t position = 0;
    skipWhitespace(body, position);
    if (position >= body.size() || body[position++] != '{') return false;

    bool first = true;
    while (true)
    {
        skipWhitespace(body, position);
        if (position < body.size() && body[position] == '}')
        {
            ++position;
            break;
        }

        if (!first)
        {
            if (position >= body.size() || body[position++] != ',')
                return false;
        }

        std::string key;
        if (!parseJsonString(body, position, key, 64U)) return false;
        if (json.strings.count(key) != 0U ||
            json.unsigneds.count(key) != 0U)
        {
            return false;
        }

        skipWhitespace(body, position);
        if (position >= body.size() || body[position++] != ':') return false;
        skipWhitespace(body, position);
        if (position >= body.size()) return false;

        if (body[position] == '"')
        {
            std::string value;
            if (!parseJsonString(body, position, value, 256U)) return false;
            json.strings.emplace(std::move(key), std::move(value));
        }
        else
        {
            std::uint64_t value = 0;
            if (!parseJsonUnsigned(body, position, value)) return false;
            json.unsigneds.emplace(std::move(key), value);
        }

        first = false;
    }

    skipWhitespace(body, position);
    return position == body.size();
}

bool exactKeys(
    const FlatJson& json,
    std::initializer_list<const char*> stringKeys,
    std::initializer_list<const char*> unsignedKeys)
{
    if (json.strings.size() != stringKeys.size() ||
        json.unsigneds.size() != unsignedKeys.size())
    {
        return false;
    }

    for (const char* key : stringKeys)
    {
        if (json.strings.count(key) == 0U) return false;
    }
    for (const char* key : unsignedKeys)
    {
        if (json.unsigneds.count(key) == 0U) return false;
    }
    return true;
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
                escaped.push_back(static_cast<char>(character));
            break;
        }
    }
    return escaped;
}

const char* controlSemantic(std::uint8_t code)
{
    switch (code)
    {
    case 1: return "autostart";
    case 2: return "present";
    case 3: return "destroy";
    case 4: return "kill";
    case 5: return "prefetch";
    case 6: return "remote";
    case 7: return "disabled";
    case 8: return "playback-autostart";
    default: return "reserved";
    }
}

const char* sessionStateName(BroadcastApplicationSessionState state)
{
    switch (state)
    {
        case BroadcastApplicationSessionState::Requested: return "requested";
        case BroadcastApplicationSessionState::Starting: return "starting";
        case BroadcastApplicationSessionState::Active: return "active";
        case BroadcastApplicationSessionState::Degraded: return "degraded";
        case BroadcastApplicationSessionState::Suspended: return "suspended";
        case BroadcastApplicationSessionState::Closing: return "closing";
        case BroadcastApplicationSessionState::Closed: return "closed";
        case BroadcastApplicationSessionState::Expired: return "expired";
        case BroadcastApplicationSessionState::Failed: return "failed";
    }
    return "failed";
}

const char* mediaStateName(HbbtvMediaSourceState state)
{
    switch (state)
    {
        case HbbtvMediaSourceState::None: return "none";
        case HbbtvMediaSourceState::Streaming: return "streaming";
        case HbbtvMediaSourceState::Paused: return "paused";
        case HbbtvMediaSourceState::Stopped: return "stopped";
        case HbbtvMediaSourceState::Failed: return "failed";
    }
    return "failed";
}

bool inputAction(
    const std::string& value,
    SuiteBridgeHbbtvInputAction& action)
{
    static const std::map<std::string, SuiteBridgeHbbtvInputAction> actions = {
        {"up", SuiteBridgeHbbtvInputAction::Up},
        {"down", SuiteBridgeHbbtvInputAction::Down},
        {"left", SuiteBridgeHbbtvInputAction::Left},
        {"right", SuiteBridgeHbbtvInputAction::Right},
        {"ok", SuiteBridgeHbbtvInputAction::Ok},
        {"back", SuiteBridgeHbbtvInputAction::Back},
        {"red", SuiteBridgeHbbtvInputAction::Red},
        {"green", SuiteBridgeHbbtvInputAction::Green},
        {"yellow", SuiteBridgeHbbtvInputAction::Yellow},
        {"blue", SuiteBridgeHbbtvInputAction::Blue},
        {"0", SuiteBridgeHbbtvInputAction::Digit0},
        {"1", SuiteBridgeHbbtvInputAction::Digit1},
        {"2", SuiteBridgeHbbtvInputAction::Digit2},
        {"3", SuiteBridgeHbbtvInputAction::Digit3},
        {"4", SuiteBridgeHbbtvInputAction::Digit4},
        {"5", SuiteBridgeHbbtvInputAction::Digit5},
        {"6", SuiteBridgeHbbtvInputAction::Digit6},
        {"7", SuiteBridgeHbbtvInputAction::Digit7},
        {"8", SuiteBridgeHbbtvInputAction::Digit8},
        {"9", SuiteBridgeHbbtvInputAction::Digit9},
        {"play", SuiteBridgeHbbtvInputAction::Play},
        {"pause", SuiteBridgeHbbtvInputAction::Pause},
        {"stop", SuiteBridgeHbbtvInputAction::Stop},
        {"fast_forward", SuiteBridgeHbbtvInputAction::FastForward},
        {"rewind", SuiteBridgeHbbtvInputAction::Rewind},
    };

    const auto found = actions.find(value);
    if (found == actions.end()) return false;
    action = found->second;
    return true;
}

ApiResponse errorResponse(int statusCode, const std::string& code)
{
    ApiResponse response;
    response.statusCode = statusCode;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body =
        "{\"error\":{\"code\":\"" + jsonEscape(code) + "\"}}";
    return response;
}

int statusForDomainError(const std::string& error)
{
    if (error == "hbbtv_backend_id_required" ||
        error == "hbbtv_channel_id_required" ||
        error == "hbbtv_discovery_request_invalid" ||
        error == "hbbtv_session_request_invalid" ||
        error == "hbbtv_input_action_invalid" ||
        error == "hbbtv_close_reason_invalid")
    {
        return 400;
    }
    if (error == "hbbtv_backend_not_found" ||
        error == "hbbtv_channel_not_in_backend_snapshot" ||
        error == "hbbtv_session_not_found")
    {
        return 404;
    }
    if (error == "hbbtv_launch_not_authorized" ||
        error == "hbbtv_input_not_authorized" ||
        error == "hbbtv_session_manage_not_authorized" ||
        error == "hbbtv_session_owner_mismatch")
    {
        return 403;
    }
    if (error == "hbbtv_backend_generation_mismatch" ||
        error == "hbbtv_backend_disabled" ||
        error == "hbbtv_application_context_stale" ||
        error == "hbbtv_runtime_discovery_stale" ||
        error == "hbbtv_session_not_active" ||
        error == "hbbtv_session_expired" ||
        error == "hbbtv_session_backend_mismatch")
    {
        return 409;
    }
    if (error == "hbbtv_discovery_payload_invalid" ||
        error == "hbbtv_application_descriptor_invalid" ||
        error == "hbbtv_runtime_payload_invalid" ||
        error == "hbbtv_runtime_identity_mismatch" ||
        error == "hbbtv_runtime_action_mismatch")
    {
        return 502;
    }
    return 503;
}

ApiResponse serializeDiscovery(
    const BroadcastApplicationDiscoverySnapshot& snapshot)
{
    if (!snapshot.error.empty())
    {
        return errorResponse(statusForDomainError(snapshot.error), snapshot.error);
    }
    if (!snapshot.payloadValid)
    {
        return errorResponse(502, "hbbtv_discovery_payload_invalid");
    }

    std::ostringstream json;
    json << "{\"schemaVersion\":1"
         << ",\"result\":\"" << jsonEscape(snapshot.result) << "\""
         << ",\"available\":"
         << (!snapshot.applications.empty() ? "true" : "false")
         << ",\"receiverActive\":"
         << (snapshot.receiverActive ? "true" : "false")
         << ",\"backendId\":\"" << jsonEscape(snapshot.backendId) << "\""
         << ",\"backendGeneration\":" << snapshot.backendGeneration
         << ",\"channelId\":\"" << jsonEscape(snapshot.channelId) << "\""
         << ",\"discoveryRevision\":" << snapshot.revision
         << ",\"observedAt\":" << snapshot.observedAt
         << ",\"applications\":[";

    for (std::size_t index = 0; index < snapshot.applications.size(); ++index)
    {
        if (index != 0U) json << ',';
        const BroadcastApplicationDescriptor& application =
            snapshot.applications[index];
        json << "{\"ref\":{\"applicationId\":"
             << application.ref.applicationId
             << ",\"descriptorRevision\":"
             << application.ref.descriptorRevision
             << "},\"control\":{\"code\":"
             << static_cast<unsigned>(application.controlCode)
             << ",\"semantic\":\""
             << controlSemantic(application.controlCode)
             << "\"},\"priority\":"
             << static_cast<unsigned>(application.priority)
             << ",\"name\":\"" << jsonEscape(application.name) << "\"}";
    }
    json << "]}";

    ApiResponse response;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}

ApiResponse serializeSession(
    const BroadcastApplicationSessionResult& result,
    int successStatus)
{
    if (!result.accepted)
    {
        const std::string error = result.error.empty()
            ? "hbbtv_session_operation_failed"
            : result.error;
        return errorResponse(statusForDomainError(error), error);
    }

    const BroadcastApplicationSession& session = result.session;
    std::ostringstream json;
    json << "{\"schemaVersion\":1"
         << ",\"sessionId\":\""
         << jsonEscape(session.broadcastApplicationSessionId) << "\""
         << ",\"state\":\"" << sessionStateName(session.state) << "\""
         << ",\"backendId\":\"" << jsonEscape(session.backendId) << "\""
         << ",\"backendGeneration\":" << session.backendGeneration
         << ",\"channelId\":\""
         << jsonEscape(session.application.channelId) << "\""
         << ",\"applicationId\":" << session.application.applicationId
         << ",\"descriptorRevision\":"
         << session.applicationDescriptorRevision
         << ",\"createdAt\":" << session.createdAt
         << ",\"expiresAt\":" << session.expiresAt
         << ",\"capabilities\":{\"status\":"
         << (session.runtimeCapabilityProfile.status ? "true" : "false")
         << ",\"close\":"
         << (session.runtimeCapabilityProfile.close ? "true" : "false")
         << ",\"inputActions\":[";

    for (std::size_t index = 0;
         index < session.runtimeCapabilityProfile.inputActions.size();
         ++index)
    {
        if (index != 0U) json << ',';
        json << "\""
             << jsonEscape(session.runtimeCapabilityProfile.inputActions[index])
             << "\"";
    }

    json << "]}";
    if (!session.closeReason.empty())
    {
        json << ",\"closeReason\":\""
             << jsonEscape(session.closeReason) << "\"";
    }
    json << '}';

    ApiResponse response;
    response.statusCode = successStatus;
    response.contentType = "application/json; charset=utf-8";
    response.headers["Cache-Control"] = "no-store";
    response.body = json.str();
    return response;
}

} // namespace

HbbtvApiRuntime& HbbtvApiRuntime::instance()
{
    static HbbtvApiRuntime runtime;
    return runtime;
}

bool HbbtvApiRuntime::configure(
    IHbbtvApplicationDiscoveryService& readService,
    HbbtvApplicationSessionService& sessionService,
    PresentationLookup presentationLookup,
    MediaLookup mediaLookup)
{
    readService_ = &readService;
    sessionService_ = &sessionService;
    presentationLookup_ = std::move(presentationLookup);
    mediaLookup_ = std::move(mediaLookup);
    return true;
}

void HbbtvApiRuntime::reset()
{
    mediaLookup_ = {};
    presentationLookup_ = {};
    sessionService_ = nullptr;
    readService_ = nullptr;
}

bool HbbtvApiRuntime::configured() const
{
    return readService_ != nullptr && sessionService_ != nullptr;
}

bool HbbtvApiRuntime::tryHandleGet(
    const std::string& requestTarget,
    ApiResponse& response,
    const std::string& actorRef,
    const std::string& clientRef) const
{
    const std::string path = requestPath(requestTarget);
    if (path != ApplicationsRoute &&
        path != SessionPresentationRoute &&
        path != SessionMediaRoute)
        return false;

    if (readService_ == nullptr || sessionService_ == nullptr)
    {
        response = errorResponse(503, "hbbtv_runtime_unavailable");
        return true;
    }

    if (path == ApplicationsRoute)
    {
        HbbtvRequest request;
        if (!parseRequest(requestTarget, request))
        {
            response = errorResponse(400, "hbbtv_request_invalid");
            return true;
        }

        response = serializeDiscovery(
            readService_->discoverApplications(
                request.backendId,
                request.channelId));
        return true;
    }

    if (!validIdentity(actorRef) ||
        (!clientRef.empty() && !validIdentity(clientRef)))
    {
        response = errorResponse(403, "hbbtv_request_context_invalid");
        return true;
    }

    const std::string client =
        clientRef.empty() ? actorRef : clientRef;

    if (path == SessionMediaRoute)
    {
        HbbtvMediaRequest mediaRequest;
        if (!parseMediaRequest(requestTarget, mediaRequest))
        {
            response = errorResponse(400, "hbbtv_media_request_invalid");
            return true;
        }

        const BroadcastApplicationSessionResult access =
            sessionService_->authorizePresentation(
                mediaRequest.sessionId,
                actorRef,
                client);
        if (!access.accepted)
        {
            response = errorResponse(
                statusForDomainError(access.error),
                access.error);
            return true;
        }
        if (access.session.backendId != mediaRequest.backendId)
        {
            response = errorResponse(409, "hbbtv_session_backend_mismatch");
            return true;
        }

        IHbbtvMediaSourceResolver* source =
            mediaLookup_ ? mediaLookup_(mediaRequest.backendId) : nullptr;
        if (source == nullptr)
        {
            response = errorResponse(503, "hbbtv_media_unavailable");
            return true;
        }

        const HbbtvMediaSource media =
            source->resolveMedia(mediaRequest.sessionId);
        if (!media.error.empty())
        {
            response = errorResponse(
                media.error == "hbbtv_media_payload_invalid" ? 502 : 503,
                media.error);
            return true;
        }

        response = ApiResponse{};
        response.statusCode = 200;
        response.contentType = "application/json; charset=utf-8";
        response.headers["Cache-Control"] = "no-store";

        std::ostringstream json;
        json << "{\"schemaVersion\":1"
             << ",\"sessionId\":\"" << jsonEscape(mediaRequest.sessionId) << "\""
             << ",\"backendId\":\"" << jsonEscape(mediaRequest.backendId) << "\""
             << ",\"available\":" << (media.available ? "true" : "false")
             << ",\"state\":\"" << mediaStateName(media.state) << "\""
             << ",\"mediaRevision\":" << media.mediaRevision
             << ",\"fullscreen\":" << (media.fullscreen ? "true" : "false")
             << ",\"geometry\":{\"x\":" << media.x
             << ",\"y\":" << media.y
             << ",\"width\":" << media.width
             << ",\"height\":" << media.height
             << "}}";
        response.body = json.str();
        return true;
    }

    HbbtvPresentationRequest request;
    if (!parsePresentationRequest(requestTarget, request))
    {
        response = errorResponse(
            400,
            "hbbtv_presentation_request_invalid");
        return true;
    }

    const BroadcastApplicationSessionResult access =
        sessionService_->authorizePresentation(
            request.sessionId,
            actorRef,
            client);
    if (!access.accepted)
    {
        response = errorResponse(
            statusForDomainError(access.error),
            access.error);
        return true;
    }
    if (access.session.backendId != request.backendId)
    {
        response = errorResponse(
            409,
            "hbbtv_session_backend_mismatch");
        return true;
    }

    IHbbtvPresentationSource* source =
        presentationLookup_
            ? presentationLookup_(request.backendId)
            : nullptr;
    if (source == nullptr)
    {
        response = errorResponse(
            503,
            "hbbtv_presentation_unavailable");
        return true;
    }

    const HbbtvPresentationFrame frame =
        source->readPresentation(
            request.sessionId,
            request.knownRevision);

    if (!frame.error.empty())
    {
        if (frame.error == "hbbtv_presentation_changed")
        {
            response = ApiResponse{};
            response.statusCode = 204;
            response.contentType = "image/qoi";
            response.headers["Cache-Control"] = "no-store";
            return true;
        }
        response = errorResponse(
            frame.error == "hbbtv_presentation_payload_invalid" ||
                    frame.error == "hbbtv_presentation_qoi_invalid"
                ? 502 : 503,
            frame.error);
        return true;
    }

    if (!frame.available || frame.unchanged)
    {
        response = ApiResponse{};
        response.statusCode = 204;
        response.contentType = "image/qoi";
        response.headers["Cache-Control"] = "no-store";
        if (frame.frameRevision != 0)
            response.headers["X-Vdr-Suite-Hbbtv-Revision"] =
                std::to_string(frame.frameRevision);
        return true;
    }

    response.statusCode = 200;
    response.contentType = "image/qoi";
    response.headers["Cache-Control"] = "no-store";
    response.headers["X-Vdr-Suite-Hbbtv-Revision"] =
        std::to_string(frame.frameRevision);
    response.headers["X-Vdr-Suite-Hbbtv-Width"] =
        std::to_string(frame.renderWidth);
    response.headers["X-Vdr-Suite-Hbbtv-Height"] =
        std::to_string(frame.renderHeight);
    response.body = frame.qoi;
    return true;
}

bool HbbtvApiRuntime::tryHandlePost(
    const std::string& requestTarget,
    const std::string& body,
    const std::string& actorRef,
    const std::string& clientRef,
    const std::string& correlationRef,
    ApiResponse& response) const
{
    const std::string path = requestPath(requestTarget);
    if (path != SessionsRoute &&
        path != SessionStatusRoute &&
        path != SessionInputRoute &&
        path != SessionCloseRoute)
    {
        return false;
    }

    if (readService_ == nullptr || sessionService_ == nullptr)
    {
        response = errorResponse(503, "hbbtv_runtime_unavailable");
        return true;
    }

    if (!validIdentity(actorRef))
    {
        response = errorResponse(403, "hbbtv_actor_context_invalid");
        return true;
    }

    if ((!clientRef.empty() && !validIdentity(clientRef)) ||
        (!correlationRef.empty() && !validIdentity(correlationRef)))
    {
        response = errorResponse(403, "hbbtv_request_context_invalid");
        return true;
    }

    const std::string client =
        clientRef.empty() ? actorRef : clientRef;
    const std::string correlation =
        correlationRef.empty() ? actorRef : correlationRef;

    FlatJson json;
    if (!parseFlatJson(body, json))
    {
        response = errorResponse(400, "hbbtv_session_request_invalid");
        return true;
    }

    if (path == SessionsRoute)
    {
        if (!exactKeys(
                json,
                {"backendId", "channelId"},
                {"applicationId", "descriptorRevision"}))
        {
            response = errorResponse(400, "hbbtv_session_request_invalid");
            return true;
        }

        const std::string& backendId = json.strings.at("backendId");
        const std::string& channelId = json.strings.at("channelId");
        const std::uint64_t applicationId =
            json.unsigneds.at("applicationId");
        const std::uint64_t descriptorRevision =
            json.unsigneds.at("descriptorRevision");

        if (!validBackendId(backendId) ||
            !validChannelId(channelId) ||
            applicationId == 0 ||
            applicationId > std::numeric_limits<std::uint32_t>::max() ||
            descriptorRevision == 0)
        {
            response = errorResponse(400, "hbbtv_session_request_invalid");
            return true;
        }

        const BroadcastApplicationDiscoverySnapshot snapshot =
            readService_->discoverApplications(backendId, channelId);
        if (!snapshot.error.empty())
        {
            response = errorResponse(
                statusForDomainError(snapshot.error),
                snapshot.error);
            return true;
        }
        if (!snapshot.payloadValid)
        {
            response = errorResponse(502, "hbbtv_discovery_payload_invalid");
            return true;
        }

        const auto application = std::find_if(
            snapshot.applications.begin(),
            snapshot.applications.end(),
            [applicationId, descriptorRevision](
                const BroadcastApplicationDescriptor& descriptor) {
                return descriptor.ref.applicationId == applicationId &&
                    descriptor.ref.descriptorRevision == descriptorRevision;
            });

        if (application == snapshot.applications.end())
        {
            response = errorResponse(
                409,
                "hbbtv_application_context_stale");
            return true;
        }

        BroadcastApplicationLaunchRequest launch;
        launch.actorId = actorRef;
        launch.clientContext = client;
        launch.correlationContext = correlation;
        launch.application = application->ref;
        launch.lifetimeSeconds = 3600;

        response = serializeSession(
            sessionService_->launch(launch),
            202);
        return true;
    }

    if (path == SessionInputRoute)
    {
        if (!exactKeys(json, {"backendId", "sessionId", "action"}, {}))
        {
            response = errorResponse(400, "hbbtv_session_request_invalid");
            return true;
        }
    }
    else if (!exactKeys(json, {"backendId", "sessionId"}, {}))
    {
        response = errorResponse(400, "hbbtv_session_request_invalid");
        return true;
    }

    const std::string& backendId = json.strings.at("backendId");
    const std::string& sessionId = json.strings.at("sessionId");
    if (!validBackendId(backendId) || !validIdentity(sessionId))
    {
        response = errorResponse(400, "hbbtv_session_request_invalid");
        return true;
    }

    const auto existing = sessionService_->find(sessionId);
    if (!existing.has_value())
    {
        response = errorResponse(404, "hbbtv_session_not_found");
        return true;
    }
    if (existing->backendId != backendId)
    {
        response = errorResponse(409, "hbbtv_session_backend_mismatch");
        return true;
    }

    if (path == SessionStatusRoute)
    {
        response = serializeSession(
            sessionService_->refresh(sessionId, actorRef, client),
            200);
        return true;
    }

    if (path == SessionInputRoute)
    {
        SuiteBridgeHbbtvInputAction action =
            SuiteBridgeHbbtvInputAction::None;
        if (!inputAction(json.strings.at("action"), action))
        {
            response = errorResponse(400, "hbbtv_input_action_invalid");
            return true;
        }

        response = serializeSession(
            sessionService_->input(
                sessionId,
                actorRef,
                client,
                action),
            200);
        return true;
    }

    response = serializeSession(
        sessionService_->close(
            sessionId,
            actorRef,
            client,
            "user_close"),
        202);
    return true;
}
