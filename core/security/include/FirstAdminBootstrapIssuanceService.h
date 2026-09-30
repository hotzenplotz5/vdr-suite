#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>

class FirstAdminBootstrapRepository;

enum class FirstAdminBootstrapIssuanceStatus
{
    issued,
    invalidLifetime,
    entropyUnavailable,
    hashingUnavailable,
    claimed,
    conflict,
    storageError,
};

struct IssuedFirstAdminBootstrap
{
    std::string bootstrapId;
    std::string setupSecret;
    std::string expiresAt;

    IssuedFirstAdminBootstrap() = default;
    ~IssuedFirstAdminBootstrap();

    IssuedFirstAdminBootstrap(const IssuedFirstAdminBootstrap&) = delete;
    IssuedFirstAdminBootstrap& operator=(const IssuedFirstAdminBootstrap&) = delete;

    IssuedFirstAdminBootstrap(IssuedFirstAdminBootstrap&& other) noexcept;
    IssuedFirstAdminBootstrap& operator=(IssuedFirstAdminBootstrap&& other) noexcept;

    void clearSecret() noexcept;
};

struct FirstAdminBootstrapIssuanceResult
{
    FirstAdminBootstrapIssuanceStatus status =
        FirstAdminBootstrapIssuanceStatus::storageError;
    std::optional<IssuedFirstAdminBootstrap> bootstrap;
};

class FirstAdminBootstrapIssuanceService
{
public:
    static constexpr int MinimumLifetimeSeconds = 300;
    static constexpr int MaximumLifetimeSeconds = 3600;
    static constexpr int DefaultLifetimeSeconds = 900;

    using EntropySource =
        std::function<bool(unsigned char*, std::size_t)>;
    using Clock =
        std::function<std::chrono::system_clock::time_point()>;

    explicit FirstAdminBootstrapIssuanceService(
        FirstAdminBootstrapRepository& repository,
        EntropySource entropySource = {},
        Clock clock = {});

    FirstAdminBootstrapIssuanceResult issue(
        int lifetimeSeconds = DefaultLifetimeSeconds);

private:
    FirstAdminBootstrapRepository& repository_;
    EntropySource entropySource_;
    Clock clock_;
};
