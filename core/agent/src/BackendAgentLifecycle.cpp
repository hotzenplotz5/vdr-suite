#include "BackendAgentLifecycle.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "AuthorizationService.h"
#include "BackendRegistryService.h"
#include "CredentialVerifierRepository.h"
#include "Database.h"
#include "SecurityIdentityProvisioningRepository.h"
#include "SecurityIdentityRepository.h"

#include <crypt.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <limits>
#include <random>
#include <set>
#include <sstream>

namespace
{
constexpr std::int64_t LeaseDurationSeconds = 90;
constexpr std::int64_t StaleGraceSeconds = 30;

std::string timestamp(std::int64_t value)
{
    return std::to_string(value);
}

bool constantTimeEqual(const std::string& first, const std::string& second)
{
    if (first.size() != second.size()) return false;
    unsigned char difference = 0;
    for (std::size_t index = 0; index < first.size(); ++index)
    {
        difference |= static_cast<unsigned char>(first[index] ^ second[index]);
    }
    return difference == 0;
}

std::string cryptSalt()
{
    static const char Alphabet[] =
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::random_device random;
    std::string result;
    result.reserve(16);
    for (int index = 0; index < 16; ++index)
    {
        result.push_back(Alphabet[random() % (sizeof(Alphabet) - 1)]);
    }
    return result;
}

bool allowedValue(
    const std::string& value,
    const std::set<std::string>& allowed)
{
    return allowed.find(value) != allowed.end();
}

bool uniqueValues(const std::vector<std::string>& values)
{
    return std::set<std::string>(values.begin(), values.end()).size() == values.size();
}


std::string observationPayloadIdentity(const std::string& canonicalPayload)
{
    std::uint64_t value = 1469598103934665603ULL;
    for (unsigned char character : canonicalPayload)
    {
        value ^= character;
        value *= 1099511628211ULL;
    }
    std::ostringstream output;
    output << std::hex << std::setw(16) << std::setfill('0') << value;
    return output.str();
}

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const { return active_; }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_;
};
}

std::string backendAgentHashSecret(const std::string& secret)
{
    if (secret.size() < 32 || secret.size() > 1024) return {};
    const std::string setting = "$6$rounds=10000$" + cryptSalt() + "$";
    crypt_data data{};
    char* encoded = crypt_r(secret.c_str(), setting.c_str(), &data);
    std::string result;
    if (encoded != nullptr && encoded[0] != '*') result = encoded;
    std::memset(&data, 0, sizeof(data));
    return result;
}

bool backendAgentVerifySecret(
    const std::string& secret,
    const std::string& hash)
{
    if (secret.empty() ||
        (hash.rfind("$6$", 0) != 0 && hash.rfind("$y$", 0) != 0))
    {
        return false;
    }
    crypt_data data{};
    char* encoded = crypt_r(secret.c_str(), hash.c_str(), &data);
    const bool accepted = encoded != nullptr && constantTimeEqual(encoded, hash);
    std::memset(&data, 0, sizeof(data));
    return accepted;
}

std::string backendAgentGenerateOpaqueId(
    const std::string& prefix,
    std::size_t randomBytes)
{
    if (prefix.empty() || randomBytes < 8 || randomBytes > 32) return {};
    std::random_device random;
    std::ostringstream output;
    output << prefix;
    for (std::size_t index = 0; index < randomBytes; ++index)
    {
        output << std::hex << std::setw(2) << std::setfill('0')
               << (random() & 0xffU);
    }
    return output.str();
}

BackendAgentLifecycleService::BackendAgentLifecycleService(
    Database& database,
    BackendAgentRepository& repository,
    BackendRegistryService& backendRegistryService,
    SecurityIdentityProvisioningRepository& provisioningRepository,
    SecurityIdentityRepository& identityRepository,
    CredentialVerifierRepository& credentialVerifierRepository,
    AccountabilityEventRepository& accountabilityRepository)
    : database_(database),
      repository_(repository),
      backendRegistryService_(backendRegistryService),
      provisioningRepository_(provisioningRepository),
      identityRepository_(identityRepository),
      credentialVerifierRepository_(credentialVerifierRepository),
      accountabilityRepository_(accountabilityRepository)
{
}

bool BackendAgentLifecycleService::safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::all_of(value.begin(), value.end(), [](unsigned char character) {
        return std::isalnum(character) || character == '-' || character == '_' ||
            character == '.' || character == ':';
    });
}

bool BackendAgentLifecycleService::safeSoftwareVersion(const std::string& value)
{
    if (value.empty() || value.size() > 128) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U || character == 0x7fU;
    });
}

bool BackendAgentLifecycleService::supportedProtocol(const std::string& version)
{
    return version == "vdr-suite-agent/1";
}

bool BackendAgentLifecycleService::validCapabilities(
    const BackendAgentCapabilityFacts& facts)
{
    static const std::set<std::string> Adapters = {
        "suitebridge", "restfulapi", "svdrp", "channels-conf"};
    static const std::set<std::string> Domains = {
        "backend-health", "channels", "epg", "recordings", "timers",
        "searchtimers", "metadata"};

    if (!facts.readOnly || facts.adapters.size() + facts.observationDomains.size() > 32 ||
        !uniqueValues(facts.adapters) || !uniqueValues(facts.observationDomains))
    {
        return false;
    }
    return std::all_of(facts.adapters.begin(), facts.adapters.end(),
                       [](const std::string& value) { return allowedValue(value, Adapters); }) &&
        std::all_of(facts.observationDomains.begin(), facts.observationDomains.end(),
                    [](const std::string& value) { return allowedValue(value, Domains); });
}

bool BackendAgentLifecycleService::appendEvent(
    const RequestSecurityContext& context,
    const std::string& eventType,
    const std::string& backendId,
    const std::string& action,
    const std::string& decision,
    const std::string& reasonCode,
    const std::string& outcome,
    std::int64_t now) const
{
    AccountabilityEvent event;
    event.eventId = backendAgentGenerateOpaqueId("aae_", 12);
    event.classes = "security,backend-agent";
    event.eventType = eventType;
    event.severity = decision == "allow" ? "info" : "warning";
    event.occurredAt = timestamp(now);
    event.actorId = context.actor.actorId;
    event.actorType = actorTypeName(context.actor.type);
    event.deviceId = context.device.has_value() ? context.device->deviceId : "";
    event.authenticationState = authenticationStateName(context.authenticationState);
    event.permission = action;
    event.backendId = backendId;
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = action;
    event.decision = decision;
    event.reasonCode = reasonCode;
    event.outcome = outcome;
    return !event.eventId.empty() && accountabilityRepository_.append(event);
}

RequestSecurityContext BackendAgentLifecycleService::systemEnrollmentContext(
    const std::string& agentId,
    const std::string& deviceId,
    const std::string& credentialId) const
{
    RequestSecurityContext context;
    context.requestId = backendAgentGenerateOpaqueId("req_", 8);
    context.correlationId = context.requestId;
    context.authenticationState = AuthenticationState::Authenticated;
    context.actor = ActorIdentity{agentId, ActorType::Agent, "Backend Agent", true};
    context.device = DeviceIdentity{deviceId, true};
    context.credential = CredentialIdentity{credentialId, true, false, false};
    return context;
}

bool BackendAgentLifecycleService::createEnrollment(
    const RequestSecurityContext& context,
    const std::string& enrollmentId,
    const std::string& backendId,
    const std::string& tokenHash,
    std::int64_t expiresAt,
    std::int64_t now,
    std::string& reasonCode)
{
    reasonCode.clear();
    AuthorizationRequest request;
    request.permission = "backend.agent.enroll";
    request.backendId = backendId;
    request.action = "backend.agent.enroll";
    const AuthorizationDecision decision = AuthorizationService().authorize(context, request);
    if (!decision.allowed)
    {
        reasonCode = decision.reasonCode;
        appendEvent(context, "agent.enrollment.denied", backendId, request.action,
                    "deny", reasonCode, "denied", now);
        return false;
    }
    if (!safeIdentifier(enrollmentId) || !safeIdentifier(backendId) ||
        !backendRegistryService_.hasBackend(backendId) ||
        (tokenHash.rfind("$6$", 0) != 0 && tokenHash.rfind("$y$", 0) != 0) ||
        expiresAt <= now)
    {
        reasonCode = "invalid_enrollment";
        appendEvent(context, "agent.enrollment.denied", backendId, request.action,
                    "deny", reasonCode, "denied", now);
        return false;
    }
    if (!appendEvent(context, "agent.enrollment.requested", backendId, request.action,
                     "allow", "authorized", "pending", now))
    {
        reasonCode = "accountability_unavailable";
        return false;
    }

    BackendAgentEnrollmentRecord enrollment;
    enrollment.enrollmentId = enrollmentId;
    enrollment.backendId = backendId;
    enrollment.tokenHash = tokenHash;
    enrollment.status = "pending";
    enrollment.expiresAt = expiresAt;
    enrollment.createdAt = now;
    if (!repository_.createEnrollment(enrollment))
    {
        const auto existing = repository_.findEnrollment(enrollmentId);
        if (existing.has_value() && existing->backendId == backendId &&
            existing->tokenHash == tokenHash && existing->expiresAt == expiresAt)
        {
            reasonCode = "enrollment_exists";
            return true;
        }
        reasonCode = "enrollment_persistence_failed";
        appendEvent(context, "agent.enrollment.denied", backendId, request.action,
                    "deny", reasonCode, "failed", now);
        return false;
    }
    reasonCode = "enrollment_created";
    return true;
}

BackendAgentEnrollmentMaterial BackendAgentLifecycleService::enroll(
    const std::string& enrollmentId,
    const std::string& enrollmentToken,
    const std::string& credentialSecret,
    std::int64_t now)
{
    BackendAgentEnrollmentMaterial result;
    const auto enrollment = repository_.findEnrollment(enrollmentId);
    RequestSecurityContext bootstrap;
    bootstrap.requestId = backendAgentGenerateOpaqueId("req_", 8);
    bootstrap.correlationId = bootstrap.requestId;
    bootstrap.authenticationState = AuthenticationState::Authenticated;
    bootstrap.actor = ActorIdentity{enrollmentId, ActorType::Agent, "Agent enrollment", true};

    if (!enrollment.has_value() || !backendAgentVerifySecret(enrollmentToken, enrollment->tokenHash))
    {
        result.reasonCode = "invalid_enrollment_credentials";
        appendEvent(bootstrap, "agent.enrollment.denied", "", "backend.agent.enroll.consume",
                    "deny", result.reasonCode, "denied", now);
        return result;
    }
    result.backendId = enrollment->backendId;
    if (!backendRegistryService_.hasBackend(enrollment->backendId))
    {
        result.reasonCode = "backend_not_found";
        appendEvent(bootstrap, "agent.enrollment.denied", enrollment->backendId,
                    "backend.agent.enroll.consume", "deny", result.reasonCode, "denied", now);
        return result;
    }
    if (enrollment->expiresAt <= now || enrollment->status == "expired")
    {
        result.reasonCode = "enrollment_expired";
        return result;
    }
    if (enrollment->status == "revoked")
    {
        result.reasonCode = "enrollment_revoked";
        return result;
    }
    if (credentialSecret.size() < 32 || credentialSecret.size() > 1024)
    {
        result.reasonCode = "invalid_credential_secret";
        return result;
    }

    if (enrollment->status == "consumed")
    {
        const auto agent = repository_.findAgent(enrollment->agentId);
        const auto verifier = credentialVerifierRepository_.findByLogin(enrollment->agentId);
        if (!agent.has_value() || agent->revoked)
        {
            result.reasonCode = "enrollment_agent_revoked";
            return result;
        }
        if (!verifier.has_value() ||
            verifier->credentialId != agent->credentialId ||
            !backendAgentVerifySecret(credentialSecret, verifier->passwordHash))
        {
            result.reasonCode = "enrollment_already_consumed";
            return result;
        }
        result.accepted = true;
        result.idempotent = true;
        result.reasonCode = "enrollment_resumed";
        result.agentId = agent->agentId;
        result.credentialId = agent->credentialId;
        result.credentialGeneration = agent->credentialGeneration;
        return result;
    }

    BackendAgentRecord agent;
    agent.agentId = backendAgentGenerateOpaqueId("agt_", 16);
    agent.backendId = enrollment->backendId;
    agent.actorId = agent.agentId;
    agent.deviceId = backendAgentGenerateOpaqueId("agd_", 16);
    agent.credentialId = backendAgentGenerateOpaqueId("agc_", 16);
    agent.credentialGeneration = 1;
    const std::string credentialHash = backendAgentHashSecret(credentialSecret);
    if (agent.agentId.empty() || agent.deviceId.empty() || agent.credentialId.empty() ||
        credentialHash.empty())
    {
        result.reasonCode = "credential_generation_failed";
        return result;
    }

    if (!appendEvent(bootstrap, "agent.enrollment.requested", agent.backendId,
                     "backend.agent.enroll.consume", "allow", "enrollment_verified",
                     "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }
    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    const RequestSecurityContext enrolledContext = systemEnrollmentContext(
        agent.agentId, agent.deviceId, agent.credentialId);
    if (!transaction.active() ||
        !provisioningRepository_.ensureTechnicalIdentity(
            agent.actorId, ActorType::Agent, "Backend Agent " + agent.backendId,
            agent.deviceId, "Backend Agent device", agent.credentialId,
            "backend-agent-basic") ||
        !credentialVerifierRepository_.ensureVerifier(
            agent.credentialId, agent.agentId, credentialHash) ||
        !repository_.bindEnrollmentInCurrentTransaction(enrollmentId, agent, now) ||
        !appendEvent(enrolledContext, "agent.enrollment.succeeded", agent.backendId,
                     "backend.agent.enroll.consume", "allow",
                     "enrollment_succeeded", "succeeded", now) ||
        !transaction.commit())
    {
        result.reasonCode = "enrollment_persistence_failed";
        appendEvent(bootstrap, "agent.enrollment.denied", agent.backendId,
                    "backend.agent.enroll.consume", "deny", result.reasonCode, "failed", now);
        return result;
    }

    result.accepted = true;
    result.reasonCode = "enrollment_succeeded";
    result.agentId = agent.agentId;
    result.credentialId = agent.credentialId;
    result.credentialGeneration = 1;
    return result;
}

bool BackendAgentLifecycleService::revokeEnrollment(
    const RequestSecurityContext& context,
    const std::string& enrollmentId,
    const std::string& reason,
    std::int64_t now,
    std::string& reasonCode)
{
    const auto enrollment = repository_.findEnrollment(enrollmentId);
    if (!enrollment.has_value())
    {
        reasonCode = "enrollment_not_found";
        return false;
    }
    AuthorizationRequest request{
        "backend.agent.revoke", enrollment->backendId, "backend.agent.enrollment.revoke"};
    const AuthorizationDecision decision = AuthorizationService().authorize(context, request);
    if (!decision.allowed)
    {
        reasonCode = decision.reasonCode;
        appendEvent(context, "agent.enrollment.revocation.denied", enrollment->backendId,
                    request.action, "deny", reasonCode, "denied", now);
        return false;
    }
    if (reason.empty() || reason.size() > 256)
    {
        reasonCode = "invalid_revocation_reason";
        return false;
    }
    if (enrollment->status == "revoked")
    {
        reasonCode = "enrollment_revoked";
        return true;
    }
    if (enrollment->status != "pending")
    {
        reasonCode = "enrollment_not_revocable";
        return false;
    }
    if (!appendEvent(context, "agent.enrollment.revocation.requested",
                     enrollment->backendId, request.action, "allow",
                     "authorized", "pending", now))
    {
        reasonCode = "accountability_unavailable";
        return false;
    }
    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active() ||
        !repository_.revokeEnrollment(enrollmentId, reason, now) ||
        !appendEvent(context, "agent.enrollment.revoked", enrollment->backendId,
                     request.action, "allow", "enrollment_revoked",
                     "succeeded", now) ||
        !transaction.commit())
    {
        reasonCode = "enrollment_revocation_persistence_failed";
        return false;
    }
    reasonCode = "enrollment_revoked";
    return true;
}

BackendAgentCredentialRotationResult BackendAgentLifecycleService::rotateCredential(
    const RequestSecurityContext& context,
    const std::string& backendId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    const std::string& rotationId,
    std::uint64_t expectedCredentialGeneration,
    const std::string& credentialSecret,
    std::int64_t now)
{
    BackendAgentCredentialRotationResult result;
    result.reasonCode = "credential_rotation_denied";
    if (!context.authenticated() || context.actor.type != ActorType::Agent)
    {
        result.reasonCode = "agent_authentication_required";
        return result;
    }

    const auto agent = repository_.findAgent(context.actor.actorId);
    if (!agent.has_value() || agent->revoked || agent->backendId != backendId ||
        !context.credential.has_value() ||
        context.credential->credentialId != agent->credentialId)
    {
        result.reasonCode = "agent_binding_mismatch";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    "backend.agent.credential.rotate", "deny",
                    result.reasonCode, "denied", now);
        return result;
    }

    AuthorizationRequest authorization{
        "backend.agent.credential.rotate", backendId,
        "backend.agent.credential.rotate"};
    const AuthorizationDecision decision =
        AuthorizationService().authorize(context, authorization);
    if (!decision.allowed)
    {
        result.reasonCode = decision.reasonCode;
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "denied", now);
        return result;
    }

    if (!safeIdentifier(agentInstanceId) ||
        agent->agentInstanceId != agentInstanceId ||
        backendGeneration == 0 ||
        agent->backendGeneration != backendGeneration)
    {
        result.reasonCode = "obsolete_agent_generation";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "denied", now);
        return result;
    }
    if (!safeIdentifier(rotationId) || expectedCredentialGeneration == 0 ||
        expectedCredentialGeneration >= static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()) ||
        credentialSecret.size() < 32 || credentialSecret.size() > 1024)
    {
        result.reasonCode = "invalid_credential_rotation";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "denied", now);
        return result;
    }

    const auto priorGeneration = repository_.credentialRotationGeneration(
        agent->agentId, rotationId);
    if (priorGeneration.has_value())
    {
        const auto verifier = credentialVerifierRepository_.findByLogin(agent->agentId);
        const bool matchesPersistedRotation =
            *priorGeneration == expectedCredentialGeneration + 1 &&
            agent->credentialGeneration == *priorGeneration &&
            verifier.has_value() && verifier->credentialId == agent->credentialId &&
            backendAgentVerifySecret(credentialSecret, verifier->passwordHash);
        if (!matchesPersistedRotation)
        {
            result.reasonCode = "credential_rotation_conflict";
            appendEvent(context, "agent.credential.rotation.result", backendId,
                        authorization.action, "deny", result.reasonCode,
                        "denied", now);
            return result;
        }
        result.accepted = true;
        result.idempotent = true;
        result.reasonCode = "credential_rotation_resumed";
        result.credentialGeneration = *priorGeneration;
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "allow", result.reasonCode,
                    "succeeded", now);
        return result;
    }

    if (agent->credentialGeneration != expectedCredentialGeneration)
    {
        result.reasonCode = "stale_credential_generation";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "denied", now);
        return result;
    }

    const std::string credentialHash = backendAgentHashSecret(credentialSecret);
    if (credentialHash.empty())
    {
        result.reasonCode = "credential_generation_failed";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "failed", now);
        return result;
    }
    if (!appendEvent(context, "agent.credential.rotation.requested", backendId,
                     authorization.action, "allow", "agent_verified",
                     "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }

    const std::uint64_t nextGeneration = expectedCredentialGeneration + 1;
    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active() ||
        !credentialVerifierRepository_.updateVerifier(
            agent->credentialId, agent->agentId, credentialHash) ||
        !repository_.recordCredentialRotationInCurrentTransaction(
            agent->agentId, rotationId, expectedCredentialGeneration,
            nextGeneration, now) ||
        !appendEvent(context, "agent.credential.rotation.result", backendId,
                     authorization.action, "allow", "credential_rotated",
                     "succeeded", now) ||
        !transaction.commit())
    {
        result.reasonCode = "credential_rotation_persistence_failed";
        appendEvent(context, "agent.credential.rotation.result", backendId,
                    authorization.action, "deny", result.reasonCode,
                    "failed", now);
        return result;
    }

    result.accepted = true;
    result.reasonCode = "credential_rotated";
    result.credentialGeneration = nextGeneration;
    return result;
}

BackendAgentConnectResult BackendAgentLifecycleService::connect(
    const RequestSecurityContext& context,
    const BackendAgentConnectRequest& request,
    std::int64_t now)
{
    BackendAgentConnectResult result;
    result.reasonCode = "connection_denied";
    if (!context.authenticated() || context.actor.type != ActorType::Agent)
    {
        result.reasonCode = "agent_authentication_required";
        return result;
    }
    const auto agent = repository_.findAgent(context.actor.actorId);
    if (!agent.has_value() || agent->revoked)
    {
        result.reasonCode = "agent_revoked_or_unknown";
        appendEvent(context, "agent.connection.denied", request.backendId,
                    "backend.agent.connect", "deny", result.reasonCode, "denied", now);
        return result;
    }
    if (agent->backendId != request.backendId ||
        !safeIdentifier(request.agentInstanceId) ||
        !safeSoftwareVersion(request.softwareVersion))
    {
        result.reasonCode = "agent_binding_mismatch";
        appendEvent(context, "agent.connection.denied", request.backendId,
                    "backend.agent.connect", "deny", result.reasonCode, "denied", now);
        return result;
    }
    if (!supportedProtocol(request.protocolVersion))
    {
        result.reasonCode = "protocol_incompatible";
        if (!appendEvent(context, "agent.connection.denied", request.backendId,
                         "backend.agent.connect", "deny", result.reasonCode,
                         "pending", now))
        {
            result.reasonCode = "accountability_unavailable";
            return result;
        }
        if (!repository_.markIncompatible(
                agent->agentId, request.protocolVersion, request.softwareVersion, now))
        {
            result.reasonCode = "connection_persistence_failed";
            appendEvent(context, "agent.connection.result", request.backendId,
                        "backend.agent.connect", "deny", result.reasonCode, "failed", now);
            return result;
        }
        appendEvent(context, "agent.connection.result", request.backendId,
                    "backend.agent.connect", "deny", "protocol_incompatible", "denied", now);
        return result;
    }
    if (!appendEvent(context, "agent.connection.accepted", request.backendId,
                     "backend.agent.connect", "allow", "agent_verified", "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }
    if (!repository_.acceptConnection(agent->agentId, request, now, result))
    {
        result.accepted = false;
        result.reasonCode = "connection_persistence_failed";
        appendEvent(context, "agent.connection.result", request.backendId,
                    "backend.agent.connect", "deny", result.reasonCode, "failed", now);
        return result;
    }
    result.reasonCode = result.disposition;
    if (result.disposition == "replace" && agent->backendGeneration > 0)
    {
        appendEvent(context, "agent.generation.replaced", request.backendId,
                    "backend.agent.connect", "allow", "new_agent_instance", "succeeded", now);
    }
    appendEvent(context, "agent.connection.result", request.backendId,
                "backend.agent.connect", result.accepted ? "allow" : "deny",
                result.reasonCode, result.accepted ? "succeeded" : "denied", now);
    return result;
}

BackendAgentHeartbeatResult BackendAgentLifecycleService::heartbeat(
    const RequestSecurityContext& context,
    const std::string& backendId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::uint64_t heartbeatSequence,
    std::int64_t now)
{
    BackendAgentHeartbeatResult result;
    if (!context.authenticated() || context.actor.type != ActorType::Agent)
    {
        result.reasonCode = "agent_authentication_required";
        return result;
    }
    if (now < 0 || now > std::numeric_limits<std::int64_t>::max() - LeaseDurationSeconds)
    {
        result.reasonCode = "invalid_control_plane_time";
        return result;
    }
    const auto agent = repository_.findAgent(context.actor.actorId);
    if (!agent.has_value() || agent->backendId != backendId)
    {
        result.reasonCode = "agent_binding_mismatch";
        appendEvent(context, "agent.lease.result", backendId,
                    "backend.agent.heartbeat", "deny", result.reasonCode, "denied", now);
        return result;
    }
    const auto current = repository_.findAgent(agent->agentId);
    const bool duplicate = current.has_value() &&
        heartbeatSequence == current->heartbeatSequence;
    if (!duplicate &&
        !appendEvent(context, "agent.lease.renewed", backendId,
                     "backend.agent.heartbeat", "allow", "lease_renewal_requested",
                     "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }
    if (!repository_.renewLease(
            agent->agentId, agentInstanceId, backendGeneration, heartbeatSequence,
            now, now + LeaseDurationSeconds, result))
    {
        result.reasonCode = "lease_persistence_failed";
        appendEvent(context, "agent.lease.result", backendId,
                    "backend.agent.heartbeat", "deny", result.reasonCode, "failed", now);
        return result;
    }
    appendEvent(context, "agent.lease.result", backendId,
                "backend.agent.heartbeat", result.accepted ? "allow" : "deny",
                result.reasonCode, result.accepted ? "succeeded" : "denied", now);
    return result;
}

BackendAgentCapabilityResult BackendAgentLifecycleService::publishCapabilities(
    const RequestSecurityContext& context,
    const std::string& backendId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::uint64_t capabilityRevision,
    const BackendAgentCapabilityFacts& facts,
    std::int64_t now)
{
    BackendAgentCapabilityResult result;
    if (!context.authenticated() || context.actor.type != ActorType::Agent)
    {
        result.reasonCode = "agent_authentication_required";
        return result;
    }
    const auto agent = repository_.findAgent(context.actor.actorId);
    if (!agent.has_value() || agent->backendId != backendId)
    {
        result.reasonCode = "agent_binding_mismatch";
        appendEvent(context, "agent.capabilities.result", backendId,
                    "backend.agent.capabilities", "deny", result.reasonCode, "denied", now);
        return result;
    }
    if (!validCapabilities(facts))
    {
        result.reasonCode = "invalid_capabilities";
        appendEvent(context, "agent.capabilities.result", backendId,
                    "backend.agent.capabilities", "deny", result.reasonCode, "denied", now);
        return result;
    }
    const auto current = repository_.findAgent(agent->agentId);
    const bool duplicate = current.has_value() &&
        capabilityRevision == current->capabilityRevision;
    if (!duplicate &&
        !appendEvent(context, "agent.capabilities.published", backendId,
                     "backend.agent.capabilities", "allow",
                     "capability_publication_requested", "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }
    if (!repository_.publishCapabilities(
            agent->agentId, agentInstanceId, backendGeneration,
            capabilityRevision, facts, now, result))
    {
        result.reasonCode = "capability_persistence_failed";
        appendEvent(context, "agent.capabilities.result", backendId,
                    "backend.agent.capabilities", "deny", result.reasonCode, "failed", now);
        return result;
    }
    appendEvent(context, "agent.capabilities.result", backendId,
                "backend.agent.capabilities", result.accepted ? "allow" : "deny",
                result.reasonCode, result.accepted ? "succeeded" : "denied", now);
    return result;
}


BackendAgentObservationResult BackendAgentLifecycleService::ingestObservation(
    const RequestSecurityContext& context,
    const BackendAgentObservationRequest& request,
    std::int64_t now)
{
    BackendAgentObservationResult result;
    if (!context.authenticated() || context.actor.type != ActorType::Agent)
    {
        result.reasonCode = "agent_authentication_required";
        return result;
    }
    const bool validEnvelope =
        supportedProtocol(request.protocolVersion) &&
        safeIdentifier(request.backendId) &&
        safeIdentifier(request.agentInstanceId) &&
        (request.observationDomain == "backend-health" ||
         request.observationDomain == "channels") &&
        request.snapshotGeneration != 0 &&
        request.producerSequence != 0 &&
        (request.kind == "completeSnapshot" || request.kind == "changeBatch") &&
        now >= 0 && now <= std::numeric_limits<std::int64_t>::max() - 300 &&
        request.capturedAt >= 0 && request.capturedAt <= now + 300 &&
        safeIdentifier(request.resourceRevision) &&
        request.observedHeartbeatSequence != 0;
    if (!validEnvelope)
    {
        result.reasonCode = "invalid_observation_envelope";
        appendEvent(context, "agent.observation.result", request.backendId,
                    "backend.agent.observation.ingest", "deny", result.reasonCode,
                    "denied", now);
        return result;
    }

    std::string canonicalPayload;
    std::string payloadIdentity;
    if (request.observationDomain == "backend-health")
    {
        if (request.agentState != "online" || !request.channels.empty() ||
            !request.upserts.empty() || !request.removedChannelIds.empty())
        {
            result.reasonCode = "invalid_backend_health_observation";
            appendEvent(context, "agent.observation.result", request.backendId,
                        "backend.agent.observation.ingest", "deny", result.reasonCode,
                        "denied", now);
            return result;
        }
        canonicalPayload =
            "agentState=" + request.agentState +
            ";heartbeatSequence=" + std::to_string(request.observedHeartbeatSequence);
        payloadIdentity = observationPayloadIdentity(canonicalPayload);
    }
    else
    {
        constexpr std::size_t MaximumChannels = 4096U;
        std::set<std::string> channelIds;
        auto validateFacts = [&](const std::vector<BackendAgentChannelFact>& facts) {
            for (const auto& fact : facts)
            {
                std::string validationReason;
                if (!backendAgentValidChannelFact(fact, validationReason) ||
                    !channelIds.insert(fact.channelId).second)
                {
                    result.reasonCode = validationReason == "channel_fact_valid"
                        ? "duplicate_channel_id"
                        : validationReason;
                    return false;
                }
            }
            return true;
        };
        bool validPayload = request.agentState.empty();
        if (request.kind == "completeSnapshot")
        {
            validPayload = validPayload && request.channels.size() <= MaximumChannels &&
                request.upserts.empty() && request.removedChannelIds.empty() &&
                validateFacts(request.channels);
        }
        else
        {
            validPayload = validPayload && request.channels.empty() &&
                request.upserts.size() <= MaximumChannels &&
                request.removedChannelIds.size() <= MaximumChannels &&
                request.upserts.size() + request.removedChannelIds.size() <= MaximumChannels &&
                (!request.upserts.empty() || !request.removedChannelIds.empty()) &&
                validateFacts(request.upserts);
            std::set<std::string> removed;
            for (const std::string& channelId : request.removedChannelIds)
            {
                if (!safeIdentifier(channelId) || !removed.insert(channelId).second ||
                    channelIds.find(channelId) != channelIds.end())
                {
                    result.reasonCode = "invalid_removed_channel_id";
                    validPayload = false;
                    break;
                }
            }
        }
        if (!validPayload)
        {
            if (result.reasonCode.empty()) result.reasonCode = "invalid_channel_observation";
            appendEvent(context, "agent.observation.result", request.backendId,
                        "backend.agent.observation.ingest", "deny", result.reasonCode,
                        "denied", now);
            return result;
        }
        canonicalPayload = backendAgentCanonicalChannelPayload(
            request.kind, request.channels, request.upserts, request.removedChannelIds);
        if (canonicalPayload.empty() || canonicalPayload.size() > 512U * 1024U)
        {
            result.reasonCode = "invalid_channel_observation_size";
            appendEvent(context, "agent.observation.result", request.backendId,
                        "backend.agent.observation.ingest", "deny", result.reasonCode,
                        "denied", now);
            return result;
        }
        payloadIdentity = backendAgentChannelPayloadIdentity(canonicalPayload);
    }

    if (!appendEvent(context, "agent.observation.requested", request.backendId,
                     "backend.agent.observation.ingest", "allow",
                     "observation_ingestion_requested", "pending", now))
    {
        result.reasonCode = "accountability_unavailable";
        return result;
    }
    if (!repository_.ingestObservation(
            context.actor.actorId, request, payloadIdentity, canonicalPayload, now, result))
    {
        result.accepted = false;
        result.replayed = false;
        result.resyncRequired = false;
        result.reasonCode = "observation_persistence_failed";
        appendEvent(context, "agent.observation.result", request.backendId,
                    "backend.agent.observation.ingest", "deny", result.reasonCode,
                    "failed", now);
        return result;
    }
    appendEvent(context, "agent.observation.result", request.backendId,
                "backend.agent.observation.ingest",
                result.accepted ? "allow" : "deny", result.reasonCode,
                result.accepted ? (result.replayed ? "replayed" : "succeeded") :
                    result.resyncRequired ? "resync-required" : "denied",
                now);
    return result;
}

BackendAgentObservationCursor BackendAgentLifecycleService::observationCursorForBackend(
    const std::string& backendId,
    const std::string& observationDomain) const
{
    return repository_.observationCursorForBackend(backendId, observationDomain);
}

bool BackendAgentLifecycleService::revoke(
    const RequestSecurityContext& context,
    const std::string& agentId,
    const std::string& reason,
    std::int64_t now,
    std::string& reasonCode)
{
    const auto agent = repository_.findAgent(agentId);
    if (!agent.has_value())
    {
        reasonCode = "agent_not_found";
        return false;
    }
    AuthorizationRequest request{"backend.agent.revoke", agent->backendId, "backend.agent.revoke"};
    const AuthorizationDecision decision = AuthorizationService().authorize(context, request);
    if (!decision.allowed)
    {
        reasonCode = decision.reasonCode;
        return false;
    }
    if (reason.empty() || reason.size() > 256)
    {
        reasonCode = "invalid_revocation_reason";
        return false;
    }
    if (!appendEvent(context, "agent.revoked", agent->backendId, request.action,
                     "allow", "authorized", "pending", now))
    {
        reasonCode = "accountability_unavailable";
        return false;
    }
    auto transactionLease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active() ||
        !repository_.revokeAgent(agentId, reason, now) ||
        !identityRepository_.revokeCredential(agent->credentialId) ||
        !identityRepository_.revokeDevice(agent->deviceId) ||
        !appendEvent(context, "agent.revoked", agent->backendId, request.action,
                     "allow", "agent_revoked", "succeeded", now) ||
        !transaction.commit())
    {
        reasonCode = "revocation_persistence_failed";
        return false;
    }
    reasonCode = "agent_revoked";
    return true;
}

BackendAgentStatus BackendAgentLifecycleService::statusForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    BackendAgentStatus status;
    const auto agent = repository_.findAgentForBackend(backendId);
    if (!agent.has_value()) return status;
    status.present = true;
    status.agentId = agent->agentId;
    status.backendId = agent->backendId;
    status.backendGeneration = agent->backendGeneration;
    status.heartbeatSequence = agent->heartbeatSequence;
    status.capabilityRevision = agent->capabilityRevision;
    status.lastHeartbeatAt = agent->lastHeartbeatAt;
    status.leaseExpiresAt = agent->leaseExpiresAt;
    status.capabilities = repository_.capabilitiesForAgent(agent->agentId);
    if (agent->revoked) status.state = BackendAgentConnectionState::Revoked;
    else if (agent->incompatible) status.state = BackendAgentConnectionState::Incompatible;
    else if (agent->backendGeneration == 0 || agent->leaseExpiresAt == 0 ||
             agent->capabilityRevision == 0)
        status.state = BackendAgentConnectionState::Offline;
    else if (now <= agent->leaseExpiresAt)
        status.state = BackendAgentConnectionState::Online;
    else if (agent->leaseExpiresAt > 0 && now >= agent->leaseExpiresAt &&
             now - agent->leaseExpiresAt <= StaleGraceSeconds)
        status.state = BackendAgentConnectionState::Stale;
    else status.state = BackendAgentConnectionState::Offline;
    return status;
}

