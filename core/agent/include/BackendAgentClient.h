#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace vdrsuite::agent
{
class IBackendAgentNativeTimerCreateTransport;
class IBackendAgentNativeTimerDeleteTransport;
class IBackendAgentNativeTimerModifyTransport;
}

struct BackendAgentClientConfig
{
    std::string controlPlaneUrl;
    std::string backendId;
    std::string identityPath;
    std::string enrollmentPath;
    std::string caCertificatePath;
    std::string channelsConfPath = "/var/lib/vdr/channels.conf";
    std::string commandStatePath = "/var/lib/vdr-suite/backend-agent/commands.state";
    std::string softwareVersion = "vdr-suite-backend-agent/1";
    std::vector<std::string> adapters;
    std::vector<std::string> observationDomains = {"backend-health"};
    std::vector<std::string> commandTypes;
    std::string suiteBridgeHost;
    int suiteBridgePort = 0;
    vdrsuite::agent::IBackendAgentNativeTimerCreateTransport*
        nativeTimerCreateTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerDeleteTransport*
        nativeTimerDeleteTransport = nullptr;
    vdrsuite::agent::IBackendAgentNativeTimerModifyTransport*
        nativeTimerModifyTransport = nullptr;
    int heartbeatIntervalSeconds = 30;
    int reconnectInitialSeconds = 1;
    int reconnectMaximumSeconds = 30;
    long connectTimeoutMilliseconds = 5000;
    long requestTimeoutMilliseconds = 10000;
};

struct BackendAgentClientState
{
    std::string agentId;
    std::string backendId;
    std::string credentialId;
    std::string credentialSecret;
    std::uint64_t credentialGeneration = 0;
    std::string pendingRotationId;
    std::string pendingCredentialSecret;
    std::uint64_t pendingCredentialGeneration = 0;
    std::uint64_t backendGeneration = 0;
    std::uint64_t heartbeatSequence = 0;
    std::uint64_t capabilityRevision = 0;
    std::uint64_t observationBackendGeneration = 0;
    std::uint64_t observationSnapshotGeneration = 0;
    std::uint64_t observationProducerSequence = 0;
    std::string pendingObservationKind;
    std::uint64_t pendingObservationSnapshotGeneration = 0;
    std::uint64_t pendingObservationProducerSequence = 0;
    std::int64_t pendingObservationCapturedAt = 0;
    std::string pendingObservationResourceRevision;
    std::uint64_t pendingObservationHeartbeatSequence = 0;
    std::uint64_t channelObservationBackendGeneration = 0;
    std::uint64_t channelObservationSnapshotGeneration = 0;
    std::uint64_t channelObservationProducerSequence = 0;
    std::string channelObservationResourceRevision;
};

struct BackendAgentEnrollmentPackage
{
    std::string enrollmentId;
    std::string backendId;
    std::string enrollmentToken;
};

struct BackendAgentTransportResponse
{
    bool transportSucceeded = false;
    int statusCode = 0;
    std::string body;
    std::string errorCode;
};

class IBackendAgentControlPlaneTransport
{
public:
    virtual ~IBackendAgentControlPlaneTransport() = default;

    virtual BackendAgentTransportResponse postEnrollment(
        const std::string& enrollmentId,
        const std::string& enrollmentToken,
        const std::string& path,
        const std::string& body) = 0;

    virtual BackendAgentTransportResponse postAuthenticated(
        const std::string& agentId,
        const std::string& credentialSecret,
        const std::string& path,
        const std::string& body) = 0;
};

class CurlBackendAgentControlPlaneTransport final
    : public IBackendAgentControlPlaneTransport
{
public:
    explicit CurlBackendAgentControlPlaneTransport(
        BackendAgentClientConfig config);

    BackendAgentTransportResponse postEnrollment(
        const std::string& enrollmentId,
        const std::string& enrollmentToken,
        const std::string& path,
        const std::string& body) override;

    BackendAgentTransportResponse postAuthenticated(
        const std::string& agentId,
        const std::string& credentialSecret,
        const std::string& path,
        const std::string& body) override;

    static bool validProtectedControlPlaneUrl(const std::string& url);

private:
    BackendAgentTransportResponse perform(
        const std::string& path,
        const std::string& body,
        const std::string& enrollmentAuthorization,
        const std::string& basicLogin,
        const std::string& basicSecret);

    BackendAgentClientConfig config_;
};

class BackendAgentClientRuntime
{
public:
    using Sleep = std::function<void(int)>;
    using Log = std::function<void(const std::string&)>;

    BackendAgentClientRuntime(
        BackendAgentClientConfig config,
        IBackendAgentControlPlaneTransport& transport,
        Sleep sleep = {},
        Log log = {});

    bool synchronize(std::string& reasonCode);
    bool rotateCredential(std::string& reasonCode);
    bool heartbeat(std::string& reasonCode);
    int run(const std::function<bool()>& stopRequested);

    const BackendAgentClientState& state() const;
    const std::string& agentInstanceId() const;

    static bool loadConfig(
        const std::string& path,
        BackendAgentClientConfig& config,
        std::string& reasonCode);
    static bool loadIdentity(
        const std::string& path,
        BackendAgentClientState& state,
        std::string& reasonCode);
    static bool loadEnrollmentPackage(
        const std::string& path,
        BackendAgentEnrollmentPackage& package,
        std::string& reasonCode);
    static bool writeIdentityAtomically(
        const std::string& path,
        const BackendAgentClientState& state,
        std::string& reasonCode);

private:
    bool enroll(std::string& reasonCode);
    bool connect(std::string& reasonCode);
    bool connectWithCredential(
        const std::string& credentialSecret,
        std::uint64_t expectedCredentialGeneration,
        std::string& reasonCode);
    bool reconcilePendingCredentialRotation(std::string& reasonCode);
    bool submitPendingCredentialRotation(
        const std::string& authenticationSecret,
        std::string& reasonCode);
    bool promotePendingCredential(std::string& reasonCode);
    bool publishCapabilities(std::string& reasonCode);
    bool publishBackendHealthObservation(std::string& reasonCode);
    bool publishChannelObservation(std::string& reasonCode);
    bool submitPendingChannelObservation(std::string& reasonCode);
    bool preparePendingChannelObservation(std::string& reasonCode);
    bool resetChannelObservationLineage(std::string& reasonCode);
    std::string pendingChannelObservationPath() const;
    bool submitPendingBackendHealthObservation(std::string& reasonCode);
    bool preparePendingBackendHealthObservation(std::string& reasonCode);
    bool promotePendingBackendHealthObservation(std::string& reasonCode);
    bool resetObservationLineage(std::string& reasonCode);
    bool persist(std::string& reasonCode);
    void log(const std::string& message) const;

    BackendAgentClientConfig config_;
    IBackendAgentControlPlaneTransport& transport_;
    Sleep sleep_;
    Log log_;
    BackendAgentClientState state_;
    std::string agentInstanceId_;
    bool synchronized_ = false;
};

bool writeBackendAgentEnrollmentPackageAtomically(
    const std::string& path,
    const BackendAgentEnrollmentPackage& package,
    std::string& reasonCode);
