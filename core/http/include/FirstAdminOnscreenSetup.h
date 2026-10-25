#pragma once

#include "FirstAdminClaimService.h"

#include <chrono>
#include <functional>
#include <mutex>
#include <string>

class FirstAdminBootstrapRepository;

enum class FirstAdminOnscreenStatus {
    ready, claimed, notReady, noDisplay, throttled, expired, rejected,
    invalid, storageError
};

struct FirstAdminOnscreenResult {
    FirstAdminOnscreenStatus status = FirstAdminOnscreenStatus::notReady;
    FirstAdminClaimStatus claimStatus = FirstAdminClaimStatus::storageError;
};

// One in-memory, short-lived physical-presence challenge per daemon.
// No additional database, identity authority, persistent PIN or service.
class FirstAdminOnscreenSetup {
public:
    using Presenter = std::function<bool(const std::string&)>;
    using Entropy = std::function<bool(unsigned char*, std::size_t)>;
    using Clock = std::function<std::chrono::steady_clock::time_point()>;

    FirstAdminOnscreenSetup(
        FirstAdminClaimService& claim,
        FirstAdminBootstrapRepository& repository,
        Presenter presenter = {},
        Entropy entropy = {},
        Clock clock = {});

    ~FirstAdminOnscreenSetup();
    FirstAdminOnscreenResult start();
    FirstAdminOnscreenResult complete(
        const std::string& code,
        FirstAdminClaimRequest request);

private:
    void clearPin() noexcept;
    FirstAdminClaimService& claim_;
    FirstAdminBootstrapRepository& repository_;
    Presenter presenter_;
    Entropy entropy_;
    Clock clock_;
    std::mutex mutex_;
    std::string pin_;
    std::chrono::steady_clock::time_point expires_{};
    std::chrono::steady_clock::time_point nextStart_{};
    int failures_ = 0;
    bool started_ = false;
};
