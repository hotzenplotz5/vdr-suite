#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

class BrowserSessionCredentialRepository;
class Database;
class SecurityIdentityRepository;

struct BrowserSessionIssuanceRequest
{
    std::string actorId;
    std::string deviceId;
    std::string issuedFromCredentialId;
    int lifetimeSeconds = 28800;
    std::size_t maximumActivePerActor = 0;
    int idleTimeoutSeconds = 0;
};

struct IssuedBrowserSession
{
    std::string tokenId;
    std::string sessionId;
    std::string credentialId;
    std::string sessionCookieValue;
    std::string csrfToken;
    std::string expiresAt;

    IssuedBrowserSession() = default;
    ~IssuedBrowserSession();

    IssuedBrowserSession(const IssuedBrowserSession&) = delete;
    IssuedBrowserSession& operator=(const IssuedBrowserSession&) = delete;
    IssuedBrowserSession(IssuedBrowserSession&& other) noexcept;
    IssuedBrowserSession& operator=(IssuedBrowserSession&& other) noexcept;

    void clearSecrets() noexcept;
};

enum class BrowserSessionIssuanceStatus
{
    Issued,
    LimitReached,
    Failed
};

struct BrowserSessionIssuanceResult
{
    BrowserSessionIssuanceStatus status =
        BrowserSessionIssuanceStatus::Failed;
    std::optional<IssuedBrowserSession> session;
};

class BrowserSessionIssuanceService
{
public:
    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    static constexpr int MinimumLifetimeSeconds = 300;
    static constexpr int DefaultLifetimeSeconds = 28800;
    static constexpr int MaximumLifetimeSeconds = 86400;
    static constexpr int MinimumIdleTimeoutSeconds = 300;
    static constexpr int MaximumIdleTimeoutSeconds = 86400;
    static constexpr std::size_t MaximumActiveSessionsPerActor = 64;

    BrowserSessionIssuanceService(
        Database& database,
        SecurityIdentityRepository& identityRepository,
        BrowserSessionCredentialRepository& credentialRepository,
        EntropySource entropySource = {},
        Clock clock = {});

    BrowserSessionIssuanceResult issueWithPolicy(
        const BrowserSessionIssuanceRequest& request);
    std::optional<IssuedBrowserSession> issue(
        const BrowserSessionIssuanceRequest& request);

private:
    Database& database_;
    SecurityIdentityRepository& identityRepository_;
    BrowserSessionCredentialRepository& credentialRepository_;
    EntropySource entropySource_;
    Clock clock_;
};
