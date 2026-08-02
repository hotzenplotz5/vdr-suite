#pragma once

#include "HttpServerResponse.h"
#include "SecurityConfiguration.h"
#include "SecurityIdentity.h"

#include <atomic>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionIssuanceService;
class BrowserSessionLifecycleService;

class BrowserSessionHttpService
{
public:
    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService,
        AccountabilityEventRepository& accountabilityRepository);

    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        BrowserSessionLifetimeConfiguration lifetimeConfiguration);

    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        BrowserSessionLifetimeConfiguration lifetimeConfiguration,
        BrowserSessionConcurrencyConfiguration concurrencyConfiguration);

    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService,
        AccountabilityEventRepository& accountabilityRepository,
        BrowserSessionLifetimeConfiguration lifetimeConfiguration,
        BrowserSessionConcurrencyConfiguration concurrencyConfiguration,
        BrowserSessionIdleConfiguration idleConfiguration);

    HttpServerResponse login(
        const RequestSecurityContext& context);
    HttpServerResponse logout(
        const RequestSecurityContext& context);

private:
    bool appendOutcome(
        const RequestSecurityContext& context,
        bool succeeded,
        const std::string& permission,
        const std::string& action,
        const std::string& reasonCode,
        const std::string& sessionId = {}) const;
    std::string opaqueId(const std::string& prefix) const;

    BrowserSessionIssuanceService& issuanceService_;
    BrowserSessionLifecycleService& lifecycleService_;
    AccountabilityEventRepository& accountabilityRepository_;
    BrowserSessionLifetimeConfiguration lifetimeConfiguration_;
    BrowserSessionConcurrencyConfiguration concurrencyConfiguration_;
    BrowserSessionIdleConfiguration idleConfiguration_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};
