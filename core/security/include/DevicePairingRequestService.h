#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class AccountabilityEventRepository;
class Database;
class DevicePairingRequestRepository;
class SecurityIdentityProvisioningRepository;
class DeviceCredentialVerifierRepository;

enum class DevicePairingIssueStatus
{
    issued,
    invalidRequest,
    entropyUnavailable,
    hashingUnavailable,
    storageError,
};

enum class DevicePairingPollStatus
{
    ok,
    invalidRequest,
    notFound,
    unauthorized,
    expired,
    consumed,
    unavailable,
};

enum class DevicePairingAdministrationStatus
{
    ok,
    invalidRequest,
    notFound,
    expired,
    revisionConflict,
    stateConflict,
    entropyUnavailable,
    storageError,
};

struct DevicePairingRequestMetadata
{
    std::string displayName;
    std::string clientKind;
    std::string appVersion;
};

struct DevicePairingResource
{
    std::string pairingRequestId;
    DevicePairingRequestMetadata client;
    std::string state;
    std::string expiresAt;
    int pollIntervalSeconds = 0;
};

struct DevicePairingAdministrativeResource
{
    DevicePairingResource resource;
    std::string resourceRevision;
    std::string decidedByActorId;
    std::string decidedAt;
};

struct IssuedDevicePairingRequest
{
    DevicePairingResource resource;
    std::string userCode;
    std::string pairingToken;

    IssuedDevicePairingRequest() = default;
    ~IssuedDevicePairingRequest();

    IssuedDevicePairingRequest(
        const IssuedDevicePairingRequest&) = delete;
    IssuedDevicePairingRequest& operator=(
        const IssuedDevicePairingRequest&) = delete;

    IssuedDevicePairingRequest(
        IssuedDevicePairingRequest&& other) noexcept;
    IssuedDevicePairingRequest& operator=(
        IssuedDevicePairingRequest&& other) noexcept;

    void clearBootstrapMaterial() noexcept;
};

struct DevicePairingIssueRequest
{
    DevicePairingRequestMetadata client;
    std::string requestId;
    std::string correlationId;
};

struct DevicePairingIssueResult
{
    DevicePairingIssueStatus status =
        DevicePairingIssueStatus::storageError;
    std::optional<IssuedDevicePairingRequest> pairing;
};

struct DevicePairingPollRequest
{
    std::string pairingRequestId;
    std::string pairingToken;
};

struct DevicePairingPollResult
{
    DevicePairingPollStatus status =
        DevicePairingPollStatus::unavailable;
    DevicePairingResource resource;
};

struct DevicePairingAdministrationContext
{
    std::string actorId;
    std::string actorType;
    std::string requestId;
    std::string correlationId;
};

struct DevicePairingAdministrationCollectionResult
{
    DevicePairingAdministrationStatus status =
        DevicePairingAdministrationStatus::storageError;
    std::vector<DevicePairingAdministrativeResource> requests;
    bool hasMore = false;
};

struct DevicePairingAdministrationReadResult
{
    DevicePairingAdministrationStatus status =
        DevicePairingAdministrationStatus::storageError;
    DevicePairingAdministrativeResource request;
};

struct DevicePairingDecisionRequest
{
    DevicePairingAdministrationContext context;
    std::string pairingRequestId;
    std::string expectedResourceRevision;
    std::string decision;
};

struct DevicePairingDecisionResult
{
    DevicePairingAdministrationStatus status =
        DevicePairingAdministrationStatus::storageError;
    DevicePairingAdministrativeResource request;
};


enum class DevicePairingCredentialIssueStatus
{
    issued,
    invalidRequest,
    notFound,
    unauthorized,
    notApproved,
    expired,
    consumed,
    entropyUnavailable,
    hashingUnavailable,
    storageError,
};

struct DevicePairingCredentialIssueRequest
{
    std::string pairingRequestId;
    std::string pairingToken;
    std::string requestId;
    std::string correlationId;
};

struct IssuedDeviceCredential
{
    std::string actorId;
    std::string deviceId;
    std::string credentialId;
    std::string credentialSecret;

    ~IssuedDeviceCredential();
    IssuedDeviceCredential() = default;
    IssuedDeviceCredential(const IssuedDeviceCredential&) = delete;
    IssuedDeviceCredential& operator=(const IssuedDeviceCredential&) = delete;
    IssuedDeviceCredential(IssuedDeviceCredential&& other) noexcept;
    IssuedDeviceCredential& operator=(IssuedDeviceCredential&& other) noexcept;
    void clearSecret() noexcept;
};

struct DevicePairingCredentialIssueResult
{
    DevicePairingCredentialIssueStatus status =
        DevicePairingCredentialIssueStatus::storageError;
    std::optional<IssuedDeviceCredential> credential;
};

class DevicePairingRequestService
{
public:
    static constexpr int LifetimeSeconds = 600;
    static constexpr int PollIntervalSeconds = 3;

    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    DevicePairingRequestService(
        Database& database,
        DevicePairingRequestRepository& repository,
        AccountabilityEventRepository& accountabilityRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    DevicePairingIssueResult issue(
        const DevicePairingIssueRequest& request);

    DevicePairingCredentialIssueResult issueDeviceCredential(
        const DevicePairingCredentialIssueRequest& request,
        SecurityIdentityProvisioningRepository& provisioning,
        DeviceCredentialVerifierRepository& verifiers);

    DevicePairingPollResult poll(
        const DevicePairingPollRequest& request) const;

    DevicePairingAdministrationCollectionResult
    listPendingForAdministration(
        const std::string& afterPairingRequestId,
        std::size_t limit) const;

    DevicePairingAdministrationReadResult
    readForAdministration(
        const std::string& pairingRequestId) const;

    DevicePairingDecisionResult decide(
        const DevicePairingDecisionRequest& request);

private:
    Database& database_;
    DevicePairingRequestRepository& repository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
