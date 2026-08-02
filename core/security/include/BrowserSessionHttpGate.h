#pragma once

#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "SecurityConfiguration.h"
#include "SecurityIdentity.h"

#include <atomic>
#include <memory>
#include <string>

class AccountabilityEventRepository;
class BrowserSessionAuthenticator;
class BrowserSessionCredentialRepository;
class SecurityPermissionGrantRepository;
class LegacyBasicAuthenticator;
class ManagedBasicAuthenticator;
class PersistentIdentityResolver;

struct BrowserSessionGateDecision
{
    bool allowed = false;
    bool login = false;
    bool logout = false;
    RequestSecurityContext context;
    HttpServerResponse rejection;
};

class BrowserSessionHttpGate
{
public:
    BrowserSessionHttpGate(
        SecurityConfiguration configuration,
        AccountabilityEventRepository& accountabilityRepository,
        const BrowserSessionCredentialRepository& credentialRepository,
        const SecurityPermissionGrantRepository& grantRepository,
        const PersistentIdentityResolver* persistentIdentityResolver,
        const ManagedBasicAuthenticator* managedBasicAuthenticator);
    ~BrowserSessionHttpGate();

    BrowserSessionHttpGate(const BrowserSessionHttpGate&) = delete;
    BrowserSessionHttpGate& operator=(const BrowserSessionHttpGate&) = delete;

    bool handles(const HttpServerRequest& request) const;
    BrowserSessionGateDecision evaluate(
        const HttpServerRequest& request) const;

private:
    RequestSecurityContext authenticateBasic(
        const HttpServerRequest& request) const;
    RequestSecurityContext authenticateBrowser(
        const HttpServerRequest& request) const;
    RequestSecurityContext resolvePersistentIdentity(
        RequestSecurityContext context) const;
    RequestSecurityContext requestContextSeed(
        const HttpServerRequest& request) const;

    bool appendDecisionEvent(
        const RequestSecurityContext& context,
        bool allowed,
        const std::string& permission,
        const std::string& action,
        const std::string& reasonCode) const;
    HttpServerResponse errorResponse(
        int statusCode,
        const std::string& code,
        const std::string& message,
        const RequestSecurityContext& context,
        bool advertiseBasic) const;
    std::string opaqueId(const std::string& prefix) const;

    SecurityConfiguration configuration_;
    AccountabilityEventRepository& accountabilityRepository_;
    const PersistentIdentityResolver* persistentIdentityResolver_;
    const ManagedBasicAuthenticator* managedBasicAuthenticator_;
    std::unique_ptr<LegacyBasicAuthenticator> legacyAuthenticator_;
    std::unique_ptr<BrowserSessionAuthenticator> browserAuthenticator_;
    mutable std::atomic<unsigned long long> idCounter_{0};
};
