#include "BackendAgentHttpServer.h"

#include "CredentialVerifierRepository.h"
#include "SecurityIdentityRepository.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace
{
constexpr std::size_t MaximumAgentBodyBytes = 16U * 1024U;
constexpr const char* AgentPathPrefix = "/api/agent/v1/";

std::int64_t unixNow()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string lowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::string headerValue(
    const std::map<std::string, std::string>& headers,
    const std::string& wanted)
{
    const std::string normalized = lowerAscii(wanted);
    for (const auto& header : headers)
    {
        if (lowerAscii(header.first) == normalized) return header.second;
    }
    return {};
}

int base64Value(unsigned char character)
{
    if (character >= 'A' && character <= 'Z') return character - 'A';
    if (character >= 'a' && character <= 'z') return character - 'a' + 26;
    if (character >= '0' && character <= '9') return character - '0' + 52;
    if (character == '+') return 62;
    if (character == '/') return 63;
    return -1;
}

bool decodeBase64(const std::string& encoded, std::string& decoded)
{
    decoded.clear();
    if (encoded.empty() || encoded.size() > 8192 || encoded.size() % 4 != 0) return false;
    for (std::size_t offset = 0; offset < encoded.size(); offset += 4)
    {
        const bool finalBlock = offset + 4 == encoded.size();
        const int first = base64Value(static_cast<unsigned char>(encoded[offset]));
        const int second = base64Value(static_cast<unsigned char>(encoded[offset + 1]));
        const int third = encoded[offset + 2] == '=' ? -2 :
            base64Value(static_cast<unsigned char>(encoded[offset + 2]));
        const int fourth = encoded[offset + 3] == '=' ? -2 :
            base64Value(static_cast<unsigned char>(encoded[offset + 3]));
        if (first < 0 || second < 0 || third == -1 || fourth == -1) return false;
        decoded.push_back(static_cast<char>((first << 2) | (second >> 4)));
        if (third == -2)
        {
            if (!finalBlock || fourth != -2 || (second & 0x0f) != 0) return false;
            continue;
        }
        decoded.push_back(static_cast<char>(((second & 0x0f) << 4) | (third >> 2)));
        if (fourth == -2)
        {
            if (!finalBlock || (third & 0x03) != 0) return false;
            continue;
        }
        decoded.push_back(static_cast<char>(((third & 0x03) << 6) | fourth));
    }
    return true;
}

bool parseBasic(
    const std::string& authorization,
    std::string& login,
    std::string& secret)
{
    login.clear();
    secret.clear();
    if (authorization.size() < 7 || lowerAscii(authorization.substr(0, 5)) != "basic" ||
        authorization[5] != ' ')
    {
        return false;
    }
    std::string decoded;
    if (!decodeBase64(authorization.substr(6), decoded)) return false;
    const std::size_t separator = decoded.find(':');
    if (separator == std::string::npos) return false;
    login = decoded.substr(0, separator);
    secret = decoded.substr(separator + 1);
    return BackendAgentLifecycleService::safeIdentifier(login) &&
        secret.size() >= 32 && secret.size() <= 1024;
}

bool parseEnrollmentAuthorization(
    const std::string& authorization,
    std::string& enrollmentId,
    std::string& token)
{
    static const std::string Prefix = "VDR-Suite-Enrollment ";
    enrollmentId.clear();
    token.clear();
    if (authorization.rfind(Prefix, 0) != 0) return false;
    const std::string value = authorization.substr(Prefix.size());
    const std::size_t separator = value.find(':');
    if (separator == std::string::npos) return false;
    enrollmentId = value.substr(0, separator);
    token = value.substr(separator + 1);
    return BackendAgentLifecycleService::safeIdentifier(enrollmentId) &&
        token.size() >= 32 && token.size() <= 1024;
}

std::size_t skipSpace(const std::string& body, std::size_t position)
{
    while (position < body.size() &&
           std::isspace(static_cast<unsigned char>(body[position]))) ++position;
    return position;
}

bool jsonString(const std::string& body, const std::string& key, std::string& value)
{
    value.clear();
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    if (position >= body.size() || body[position] != '"') return false;
    ++position;
    bool escaped = false;
    while (position < body.size())
    {
        const char character = body[position++];
        if (escaped)
        {
            switch (character)
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
            escaped = false;
            continue;
        }
        if (character == '\\')
        {
            escaped = true;
            continue;
        }
        if (character == '"') return true;
        if (static_cast<unsigned char>(character) < 0x20U) return false;
        value.push_back(character);
        if (value.size() > 4096) return false;
    }
    return false;
}

bool jsonUnsigned(const std::string& body, const std::string& key, std::uint64_t& value)
{
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    const std::size_t start = position;
    while (position < body.size() && std::isdigit(static_cast<unsigned char>(body[position])))
        ++position;
    if (start == position || position - start > 20) return false;
    const std::size_t delimiter = skipSpace(body, position);
    if (delimiter >= body.size() ||
        (body[delimiter] != ',' && body[delimiter] != '}')) return false;
    errno = 0;
    char* end = nullptr;
    const std::string encoded = body.substr(start, position - start);
    const unsigned long long parsed = std::strtoull(encoded.c_str(), &end, 10);
    if (errno == ERANGE || end == nullptr || *end != '\0' ||
        parsed > static_cast<unsigned long long>(
            std::numeric_limits<std::int64_t>::max())) return false;
    value = static_cast<std::uint64_t>(parsed);
    return true;
}

bool jsonBool(const std::string& body, const std::string& key, bool& value)
{
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    if (body.compare(position, 4, "true") == 0) { value = true; return true; }
    if (body.compare(position, 5, "false") == 0) { value = false; return true; }
    return false;
}

bool jsonStringArray(
    const std::string& body,
    const std::string& key,
    std::vector<std::string>& values)
{
    values.clear();
    const std::string needle = "\"" + key + "\"";
    std::size_t position = body.find(needle);
    if (position == std::string::npos) return false;
    position = body.find(':', position + needle.size());
    if (position == std::string::npos) return false;
    position = skipSpace(body, position + 1);
    if (position >= body.size() || body[position] != '[') return false;
    ++position;
    while (true)
    {
        position = skipSpace(body, position);
        if (position >= body.size()) return false;
        if (body[position] == ']') return true;
        if (body[position] != '"') return false;
        ++position;
        std::string value;
        while (position < body.size() && body[position] != '"')
        {
            const unsigned char character = static_cast<unsigned char>(body[position++]);
            if (character < 0x20U || character == '\\') return false;
            value.push_back(static_cast<char>(character));
            if (value.size() > 128) return false;
        }
        if (position >= body.size() || body[position] != '"') return false;
        ++position;
        values.push_back(value);
        if (values.size() > 32) return false;
        position = skipSpace(body, position);
        if (position >= body.size()) return false;
        if (body[position] == ']') return true;
        if (body[position] != ',') return false;
        ++position;
    }
}

std::string jsonEscape(const std::string& value)
{
    std::ostringstream output;
    for (unsigned char character : value)
    {
        switch (character)
        {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character >= 0x20U) output << static_cast<char>(character);
        }
    }
    return output.str();
}

HttpServerResponse jsonResponse(int statusCode, const std::string& body)
{
    HttpServerResponse response;
    response.statusCode = statusCode;
    response.headers["Content-Type"] = "application/json";
    response.headers["Cache-Control"] = "no-store";
    response.body = body;
    return response;
}

HttpServerResponse errorResponse(int statusCode, const std::string& code)
{
    return jsonResponse(
        statusCode,
        "{\"error\":{\"code\":\"" + jsonEscape(code) +
        "\",\"message\":\"Agent protocol request rejected\"}}");
}

bool isAgentPath(const std::string& path)
{
    return path.rfind(AgentPathPrefix, 0) == 0;
}
}

BackendAgentHttpServer::BackendAgentHttpServer(
    std::unique_ptr<IHttpServer> clientServer,
    BackendAgentLifecycleService& lifecycleService,
    BackendAgentRepository& repository,
    CredentialVerifierRepository& credentialVerifierRepository,
    SecurityIdentityRepository& identityRepository)
    : clientServer_(std::move(clientServer)),
      lifecycleService_(lifecycleService),
      repository_(repository),
      credentialVerifierRepository_(credentialVerifierRepository),
      identityRepository_(identityRepository)
{
}

RequestSecurityContext BackendAgentHttpServer::authenticateAgent(
    const HttpServerRequest& request) const
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_", 8);
    context.correlationId = context.requestId;

    std::string agentId;
    std::string secret;
    if (!parseBasic(headerValue(request.headers, "Authorization"), agentId, secret))
    {
        context.authenticationState = AuthenticationState::Invalid;
        return context;
    }
    const auto verifier = credentialVerifierRepository_.findByLogin(agentId);
    const auto agent = repository_.findAgent(agentId);
    const bool accepted = verifier.has_value() && agent.has_value() &&
        verifier->credentialId == agent->credentialId &&
        backendAgentVerifySecret(secret, verifier->passwordHash);
    std::fill(secret.begin(), secret.end(), '\0');
    if (!accepted)
    {
        context.authenticationState = AuthenticationState::Invalid;
        return context;
    }

    const auto actor = identityRepository_.findActor(agent->actorId);
    const auto device = identityRepository_.findDevice(agent->deviceId);
    const auto credential = identityRepository_.findCredential(agent->credentialId);
    if (!actor.has_value() || actor->type != ActorType::Agent ||
        !device.has_value() || device->actorId != actor->actorId ||
        !credential.has_value() || credential->actorId != actor->actorId)
    {
        context.authenticationState = AuthenticationState::Invalid;
        return context;
    }

    context.authenticationState =
        actor->revoked || device->revoked || credential->revoked
            ? AuthenticationState::Revoked
            : credential->expired
                ? AuthenticationState::Expired
                : AuthenticationState::Authenticated;
    context.actor = ActorIdentity{
        actor->actorId,
        actor->type,
        actor->displayName,
        actor->active && !actor->revoked};
    context.device = DeviceIdentity{
        device->deviceId,
        device->active && !device->revoked};
    context.credential = CredentialIdentity{
        credential->credentialId,
        credential->active,
        credential->expired,
        credential->revoked};
    context.grants.push_back(PermissionGrant{
        "backend.agent.credential.rotate", agent->backendId});
    context.permissionGrantResolution = PermissionGrantResolutionState::Resolved;
    return context;
}

HttpServerResponse BackendAgentHttpServer::handleEnrollment(
    const HttpServerRequest& request) const
{
    std::string enrollmentId;
    std::string token;
    if (!parseEnrollmentAuthorization(
            headerValue(request.headers, "Authorization"), enrollmentId, token))
    {
        return errorResponse(401, "agent_enrollment_authentication_required");
    }
    std::string credentialSecret;
    if (!jsonString(request.body, "credentialSecret", credentialSecret))
    {
        std::fill(token.begin(), token.end(), '\0');
        return errorResponse(400, "invalid_agent_enrollment_payload");
    }
    const BackendAgentEnrollmentMaterial result =
        lifecycleService_.enroll(enrollmentId, token, credentialSecret, unixNow());
    std::fill(token.begin(), token.end(), '\0');
    std::fill(credentialSecret.begin(), credentialSecret.end(), '\0');
    if (!result.accepted)
    {
        return errorResponse(403, result.reasonCode);
    }
    std::ostringstream body;
    body << "{\"agentId\":\"" << jsonEscape(result.agentId)
         << "\",\"backendId\":\"" << jsonEscape(result.backendId)
         << "\",\"credentialId\":\"" << jsonEscape(result.credentialId)
         << "\",\"credentialGeneration\":" << result.credentialGeneration
         << ",\"idempotent\":" << (result.idempotent ? "true" : "false")
         << "}";
    return jsonResponse(200, body.str());
}

HttpServerResponse BackendAgentHttpServer::handleCredentialRotation(
    const HttpServerRequest& request,
    const RequestSecurityContext& context) const
{
    std::string backendId;
    std::string agentInstanceId;
    std::string rotationId;
    std::string credentialSecret;
    std::uint64_t backendGeneration = 0;
    std::uint64_t expectedCredentialGeneration = 0;
    if (!jsonString(request.body, "backendId", backendId) ||
        !jsonString(request.body, "agentInstanceId", agentInstanceId) ||
        !jsonUnsigned(request.body, "backendGeneration", backendGeneration) ||
        !jsonString(request.body, "rotationId", rotationId) ||
        !jsonUnsigned(
            request.body, "expectedCredentialGeneration",
            expectedCredentialGeneration) ||
        !jsonString(request.body, "credentialSecret", credentialSecret))
    {
        std::fill(credentialSecret.begin(), credentialSecret.end(), '\0');
        return errorResponse(400, "invalid_agent_credential_rotation_payload");
    }
    const BackendAgentCredentialRotationResult result =
        lifecycleService_.rotateCredential(
            context, backendId, agentInstanceId, backendGeneration, rotationId,
            expectedCredentialGeneration, credentialSecret, unixNow());
    std::fill(credentialSecret.begin(), credentialSecret.end(), '\0');
    if (!result.accepted) return errorResponse(409, result.reasonCode);
    std::ostringstream body;
    body << "{\"credentialGeneration\":" << result.credentialGeneration
         << ",\"idempotent\":" << (result.idempotent ? "true" : "false")
         << "}";
    return jsonResponse(200, body.str());
}

HttpServerResponse BackendAgentHttpServer::handleConnect(
    const HttpServerRequest& request,
    const RequestSecurityContext& context) const
{
    BackendAgentConnectRequest connect;
    if (!jsonString(request.body, "backendId", connect.backendId) ||
        !jsonString(request.body, "agentInstanceId", connect.agentInstanceId) ||
        !jsonString(request.body, "protocolVersion", connect.protocolVersion) ||
        !jsonString(request.body, "softwareVersion", connect.softwareVersion) ||
        !jsonUnsigned(request.body, "backendGeneration", connect.claimedBackendGeneration) ||
        !jsonUnsigned(request.body, "heartbeatSequence", connect.claimedHeartbeatSequence) ||
        !jsonUnsigned(request.body, "capabilityRevision", connect.claimedCapabilityRevision))
    {
        return errorResponse(400, "invalid_agent_connect_payload");
    }
    const BackendAgentConnectResult result = lifecycleService_.connect(context, connect, unixNow());
    if (!result.accepted) return errorResponse(409, result.reasonCode);
    std::ostringstream body;
    body << "{\"agentId\":\"" << jsonEscape(result.agentId)
         << "\",\"backendId\":\"" << jsonEscape(result.backendId)
         << "\",\"backendGeneration\":" << result.backendGeneration
         << ",\"credentialGeneration\":" << result.credentialGeneration
         << ",\"heartbeatSequence\":" << result.heartbeatSequence
         << ",\"capabilityRevision\":" << result.capabilityRevision
         << ",\"leaseDurationSeconds\":" << result.leaseDurationSeconds
         << ",\"disposition\":\"" << jsonEscape(result.disposition) << "\"}";
    return jsonResponse(200, body.str());
}

HttpServerResponse BackendAgentHttpServer::handleHeartbeat(
    const HttpServerRequest& request,
    const RequestSecurityContext& context) const
{
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t generation = 0;
    std::uint64_t sequence = 0;
    if (!jsonString(request.body, "backendId", backendId) ||
        !jsonString(request.body, "agentInstanceId", agentInstanceId) ||
        !jsonUnsigned(request.body, "backendGeneration", generation) ||
        !jsonUnsigned(request.body, "heartbeatSequence", sequence))
    {
        return errorResponse(400, "invalid_agent_heartbeat_payload");
    }
    const BackendAgentHeartbeatResult result = lifecycleService_.heartbeat(
        context, backendId, agentInstanceId, generation, sequence, unixNow());
    if (!result.accepted) return errorResponse(409, result.reasonCode);
    std::ostringstream body;
    body << "{\"heartbeatSequence\":" << result.heartbeatSequence
         << ",\"leaseExpiresAt\":" << result.leaseExpiresAt
         << ",\"duplicate\":" << (result.duplicate ? "true" : "false") << "}";
    return jsonResponse(200, body.str());
}

HttpServerResponse BackendAgentHttpServer::handleCapabilities(
    const HttpServerRequest& request,
    const RequestSecurityContext& context) const
{
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t generation = 0;
    std::uint64_t revision = 0;
    BackendAgentCapabilityFacts facts;
    if (!jsonString(request.body, "backendId", backendId) ||
        !jsonString(request.body, "agentInstanceId", agentInstanceId) ||
        !jsonUnsigned(request.body, "backendGeneration", generation) ||
        !jsonUnsigned(request.body, "capabilityRevision", revision) ||
        !jsonBool(request.body, "readOnly", facts.readOnly) ||
        !jsonStringArray(request.body, "adapters", facts.adapters) ||
        !jsonStringArray(request.body, "observationDomains", facts.observationDomains))
    {
        return errorResponse(400, "invalid_agent_capability_payload");
    }
    const BackendAgentCapabilityResult result = lifecycleService_.publishCapabilities(
        context, backendId, agentInstanceId, generation, revision, facts, unixNow());
    if (!result.accepted) return errorResponse(409, result.reasonCode);
    std::ostringstream body;
    body << "{\"capabilityRevision\":" << result.capabilityRevision
         << ",\"duplicate\":" << (result.duplicate ? "true" : "false") << "}";
    return jsonResponse(200, body.str());
}


HttpServerResponse BackendAgentHttpServer::handleBackendHealthObservation(
    const HttpServerRequest& request,
    const RequestSecurityContext& context) const
{
    BackendAgentObservationRequest observation;
    std::uint64_t capturedAt = 0;
    if (!jsonString(request.body, "protocolVersion", observation.protocolVersion) ||
        !jsonString(request.body, "backendId", observation.backendId) ||
        !jsonString(request.body, "agentInstanceId", observation.agentInstanceId) ||
        !jsonUnsigned(request.body, "backendGeneration", observation.backendGeneration) ||
        !jsonString(request.body, "observationDomain", observation.observationDomain) ||
        !jsonUnsigned(request.body, "snapshotGeneration", observation.snapshotGeneration) ||
        !jsonUnsigned(request.body, "producerSequence", observation.producerSequence) ||
        !jsonString(request.body, "kind", observation.kind) ||
        !jsonUnsigned(request.body, "capturedAt", capturedAt) ||
        !jsonString(request.body, "resourceRevision", observation.resourceRevision) ||
        !jsonString(request.body, "agentState", observation.agentState) ||
        !jsonUnsigned(
            request.body, "observedHeartbeatSequence",
            observation.observedHeartbeatSequence))
    {
        return errorResponse(400, "invalid_backend_health_observation_payload");
    }
    if (capturedAt > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
    {
        return errorResponse(400, "invalid_backend_health_observation_payload");
    }
    observation.capturedAt = static_cast<std::int64_t>(capturedAt);
    const BackendAgentObservationResult result =
        lifecycleService_.ingestObservation(context, observation, unixNow());
    if (!result.accepted)
    {
        return errorResponse(result.resyncRequired ? 409 : 422, result.reasonCode);
    }
    std::ostringstream body;
    body << "{\"outcome\":\"" << (result.replayed ? "replayed" : "accepted")
         << "\",\"reasonCode\":\"" << jsonEscape(result.reasonCode)
         << "\",\"snapshotGeneration\":" << result.snapshotGeneration
         << ",\"producerSequence\":" << result.producerSequence
         << ",\"lastAcceptedSequence\":" << result.lastAcceptedSequence
         << "}";
    return jsonResponse(200, body.str());
}

HttpServerResponse BackendAgentHttpServer::handleRequest(
    const HttpServerRequest& request) const
{
    if (!isAgentPath(request.path)) return clientServer_->handleRequest(request);
    if (request.body.size() > MaximumAgentBodyBytes)
        return errorResponse(413, "agent_payload_too_large");
    if (request.method != "POST") return errorResponse(405, "agent_method_not_allowed");
    if (request.path == "/api/agent/v1/enroll") return handleEnrollment(request);

    const RequestSecurityContext context = authenticateAgent(request);
    if (!context.authenticated()) return errorResponse(401, "agent_authentication_failed");
    if (request.path == "/api/agent/v1/credentials/rotate")
        return handleCredentialRotation(request, context);
    if (request.path == "/api/agent/v1/connect") return handleConnect(request, context);
    if (request.path == "/api/agent/v1/heartbeat") return handleHeartbeat(request, context);
    if (request.path == "/api/agent/v1/capabilities") return handleCapabilities(request, context);
    if (request.path == "/api/agent/v1/observations/backend-health")
        return handleBackendHealthObservation(request, context);
    return errorResponse(404, "agent_route_not_found");
}
