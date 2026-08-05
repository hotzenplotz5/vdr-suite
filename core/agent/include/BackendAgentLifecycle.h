#pragma once

#include "SecurityIdentity.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class AccountabilityEventRepository;
class AuthorizationService;
class BackendAgentRepository;
class BackendRegistryService;
class CredentialVerifierRepository;
class Database;
class SecurityIdentityProvisioningRepository;
class SecurityIdentityRepository;

enum class BackendAgentConnectionState
{
    Offline,
    Online,
    Stale,
    Revoked,
    Incompatible
};

inline std::string backendAgentConnectionStateName(
    BackendAgentConnectionState state)
{
    switch (state)
    {
        case BackendAgentConnectionState::Online: return "online";
        case BackendAgentConnectionState::Stale: return "stale";
        case BackendAgentConnectionState::Revoked: return "revoked";
        case BackendAgentConnectionState::Incompatible: return "incompatible";
        case BackendAgentConnectionState::Offline:
        default: return "offline";
    }
}

struct BackendAgentRecord
{
    std::string agentId;
    std::string backendId;
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::uint64_t credentialGeneration = 1;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string protocolVersion;
    std::string softwareVersion;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    std::int64_t lastConnectedAt = 0;
    std::int64_t lastHeartbeatAt = 0;
    std::int64_t leaseExpiresAt = 0;
    bool revoked = false;
    std::string revocationReason;
    bool incompatible = false;
};

struct BackendAgentEnrollmentRecord
{
    std::string enrollmentId;
    std::string backendId;
    std::string tokenHash;
    std::string status;
    std::int64_t expiresAt = 0;
    std::int64_t createdAt = 0;
    std::string agentId;
};

struct BackendAgentCapabilityFacts
{
    bool readOnly = true;
    std::vector<std::string> adapters;
    std::vector<std::string> observationDomains;
};

struct BackendAgentEnrollmentMaterial
{
    bool accepted = false;
    bool idempotent = false;
    std::string reasonCode;
    std::string agentId;
    std::string backendId;
    std::string credentialId;
    std::uint64_t credentialGeneration = 0;
};

struct BackendAgentConnectRequest
{
    std::string backendId;
    std::string agentInstanceId;
    std::string protocolVersion;
    std::string softwareVersion;
    std::uint64_t claimedBackendGeneration = 0;
    std::uint64_t claimedHeartbeatSequence = 0;
    std::uint64_t claimedCapabilityRevision = 0;
};

struct BackendAgentConnectResult
{
    bool accepted = false;
    std::string reasonCode;
    std::string disposition;
    std::string agentId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::uint64_t credentialGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    std::int64_t leaseDurationSeconds = 90;
};

struct BackendAgentCredentialRotationResult
{
    bool accepted = false;
    bool idempotent = false;
    std::string reasonCode;
    std::uint64_t credentialGeneration = 0;
};

struct BackendAgentHeartbeatResult
{
    bool accepted = false;
    bool duplicate = false;
    std::string reasonCode;
    std::uint64_t heartbeatSequence = 0;
    std::int64_t leaseExpiresAt = 0;
};

struct BackendAgentCapabilityResult
{
    bool accepted = false;
    bool duplicate = false;
    std::string reasonCode;
    std::uint64_t capabilityRevision = 0;
};


struct BackendAgentObservationRequest
{
    std::string protocolVersion;
    std::string backendId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::string observationDomain;
    std::uint64_t snapshotGeneration = 0;
    std::uint64_t producerSequence = 0;
    std::string kind;
    std::int64_t capturedAt = 0;
    std::string resourceRevision;
    std::string agentState;
    std::uint64_t observedHeartbeatSequence = 0;
};

struct BackendAgentObservationResult
{
    bool accepted = false;
    bool replayed = false;
    bool resyncRequired = false;
    std::string reasonCode;
    std::uint64_t snapshotGeneration = 0;
    std::uint64_t producerSequence = 0;
    std::uint64_t lastAcceptedSequence = 0;
};

struct BackendAgentObservationCursor
{
    bool present = false;
    std::string backendId;
    std::string observationDomain;
    std::string agentId;
    std::string agentInstanceId;
    std::uint64_t backendGeneration = 0;
    std::uint64_t snapshotGeneration = 0;
    std::uint64_t producerSequence = 0;
    std::string resourceRevision;
    std::string payloadIdentity;
    std::int64_t capturedAt = 0;
    std::int64_t acceptedAt = 0;
};

struct BackendAgentStatus
{
    bool present = false;
    std::string agentId;
    std::string backendId;
    std::uint64_t backendGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    std::int64_t lastHeartbeatAt = 0;
    std::int64_t leaseExpiresAt = 0;
    BackendAgentConnectionState state = BackendAgentConnectionState::Offline;
    BackendAgentCapabilityFacts capabilities;
};

class BackendAgentRepository
{
public:
    explicit BackendAgentRepository(Database& database);

    bool ensureSchema();
    bool createEnrollment(
        const BackendAgentEnrollmentRecord& enrollment);
    std::optional<BackendAgentEnrollmentRecord> findEnrollment(
        const std::string& enrollmentId) const;
    std::optional<BackendAgentRecord> findAgent(
        const std::string& agentId) const;
    std::optional<BackendAgentRecord> findAgentForBackend(
        const std::string& backendId) const;
    BackendAgentCapabilityFacts capabilitiesForAgent(
        const std::string& agentId) const;

    bool bindEnrollmentInCurrentTransaction(
        const std::string& enrollmentId,
        const BackendAgentRecord& agent,
        std::int64_t consumedAt);
    bool revokeEnrollment(
        const std::string& enrollmentId,
        const std::string& reason,
        std::int64_t revokedAt);
    std::optional<std::uint64_t> credentialRotationGeneration(
        const std::string& agentId,
        const std::string& rotationId) const;
    bool recordCredentialRotationInCurrentTransaction(
        const std::string& agentId,
        const std::string& rotationId,
        std::uint64_t fromGeneration,
        std::uint64_t toGeneration,
        std::int64_t rotatedAt);

    bool markIncompatible(
        const std::string& agentId,
        const std::string& protocolVersion,
        const std::string& softwareVersion,
        std::int64_t connectedAt);
    bool acceptConnection(
        const std::string& agentId,
        const BackendAgentConnectRequest& request,
        std::int64_t connectedAt,
        BackendAgentConnectResult& result);
    bool renewLease(
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        std::uint64_t heartbeatSequence,
        std::int64_t acceptedAt,
        std::int64_t leaseExpiresAt,
        BackendAgentHeartbeatResult& result);
    bool publishCapabilities(
        const std::string& agentId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        std::uint64_t capabilityRevision,
        const BackendAgentCapabilityFacts& facts,
        std::int64_t publishedAt,
        BackendAgentCapabilityResult& result);
    bool ingestObservation(
        const std::string& agentId,
        const BackendAgentObservationRequest& request,
        const std::string& payloadIdentity,
        const std::string& canonicalPayload,
        std::int64_t acceptedAt,
        BackendAgentObservationResult& result);
    BackendAgentObservationCursor observationCursorForBackend(
        const std::string& backendId,
        const std::string& observationDomain) const;
    bool revokeAgent(
        const std::string& agentId,
        const std::string& reason,
        std::int64_t revokedAt);

private:
    Database& database_;
};

class BackendAgentLifecycleService
{
public:
    BackendAgentLifecycleService(
        Database& database,
        BackendAgentRepository& repository,
        BackendRegistryService& backendRegistryService,
        SecurityIdentityProvisioningRepository& provisioningRepository,
        SecurityIdentityRepository& identityRepository,
        CredentialVerifierRepository& credentialVerifierRepository,
        AccountabilityEventRepository& accountabilityRepository);

    bool createEnrollment(
        const RequestSecurityContext& context,
        const std::string& enrollmentId,
        const std::string& backendId,
        const std::string& tokenHash,
        std::int64_t expiresAt,
        std::int64_t now,
        std::string& reasonCode);

    BackendAgentEnrollmentMaterial enroll(
        const std::string& enrollmentId,
        const std::string& enrollmentToken,
        const std::string& credentialSecret,
        std::int64_t now);

    bool revokeEnrollment(
        const RequestSecurityContext& context,
        const std::string& enrollmentId,
        const std::string& reason,
        std::int64_t now,
        std::string& reasonCode);

    BackendAgentCredentialRotationResult rotateCredential(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        const std::string& rotationId,
        std::uint64_t expectedCredentialGeneration,
        const std::string& credentialSecret,
        std::int64_t now);

    BackendAgentConnectResult connect(
        const RequestSecurityContext& context,
        const BackendAgentConnectRequest& request,
        std::int64_t now);

    BackendAgentHeartbeatResult heartbeat(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        std::uint64_t heartbeatSequence,
        std::int64_t now);

    BackendAgentCapabilityResult publishCapabilities(
        const RequestSecurityContext& context,
        const std::string& backendId,
        const std::string& agentInstanceId,
        std::uint64_t backendGeneration,
        std::uint64_t capabilityRevision,
        const BackendAgentCapabilityFacts& facts,
        std::int64_t now);
    BackendAgentObservationResult ingestObservation(
        const RequestSecurityContext& context,
        const BackendAgentObservationRequest& request,
        std::int64_t now);

    BackendAgentObservationCursor observationCursorForBackend(
        const std::string& backendId,
        const std::string& observationDomain) const;

    bool revoke(
        const RequestSecurityContext& context,
        const std::string& agentId,
        const std::string& reason,
        std::int64_t now,
        std::string& reasonCode);

    BackendAgentStatus statusForBackend(
        const std::string& backendId,
        std::int64_t now) const;

    static bool supportedProtocol(const std::string& version);
    static bool safeIdentifier(const std::string& value);
    static bool safeSoftwareVersion(const std::string& value);
    static bool validCapabilities(const BackendAgentCapabilityFacts& facts);

private:
    bool appendEvent(
        const RequestSecurityContext& context,
        const std::string& eventType,
        const std::string& backendId,
        const std::string& action,
        const std::string& decision,
        const std::string& reasonCode,
        const std::string& outcome,
        std::int64_t now) const;
    RequestSecurityContext systemEnrollmentContext(
        const std::string& agentId,
        const std::string& deviceId,
        const std::string& credentialId) const;

    Database& database_;
    BackendAgentRepository& repository_;
    BackendRegistryService& backendRegistryService_;
    SecurityIdentityProvisioningRepository& provisioningRepository_;
    SecurityIdentityRepository& identityRepository_;
    CredentialVerifierRepository& credentialVerifierRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
};

std::string backendAgentHashSecret(const std::string& secret);
bool backendAgentVerifySecret(
    const std::string& secret,
    const std::string& hash);
std::string backendAgentGenerateOpaqueId(
    const std::string& prefix,
    std::size_t randomBytes = 16);
