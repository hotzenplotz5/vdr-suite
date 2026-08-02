#pragma once

#include "SecurityConfiguration.h"

#include <atomic>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionCredentialRepository;
class Database;
class SecurityIdentityRepository;
struct TerminalBrowserSessionCandidate;

class BrowserSessionRetentionService
{
public:
    BrowserSessionRetentionService(
        Database& database,
        BrowserSessionCredentialRepository& credentialRepository,
        SecurityIdentityRepository& identityRepository,
        AccountabilityEventRepository& accountabilityRepository);

    bool cleanup(
        const BrowserSessionRetentionConfiguration& retentionConfiguration,
        const BrowserSessionIdleConfiguration& idleConfiguration);

private:
    bool appendCleanupEvent(
        const TerminalBrowserSessionCandidate& candidate);
    std::string opaqueId(const std::string& prefix) const;

    Database& database_;
    BrowserSessionCredentialRepository& credentialRepository_;
    SecurityIdentityRepository& identityRepository_;
    AccountabilityEventRepository& accountabilityRepository_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};