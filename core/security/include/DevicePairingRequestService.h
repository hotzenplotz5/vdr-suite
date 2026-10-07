#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

class AccountabilityEventRepository;
class Database;
class DevicePairingRequestRepository;

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
    unavailable,
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

    DevicePairingPollResult poll(
        const DevicePairingPollRequest& request) const;

private:
    Database& database_;
    DevicePairingRequestRepository& repository_;
    AccountabilityEventRepository& accountabilityRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
