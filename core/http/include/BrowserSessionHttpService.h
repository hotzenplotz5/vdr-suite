#pragma once

#include "HttpServerResponse.h"
#include "SecurityConfiguration.h"
#include "SecurityIdentity.h"

class BrowserSessionIssuanceService;
class BrowserSessionLifecycleService;

class BrowserSessionHttpService
{
public:
    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService);

    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService,
        BrowserSessionLifetimeConfiguration lifetimeConfiguration);

    HttpServerResponse login(
        const RequestSecurityContext& context);
    HttpServerResponse logout(
        const RequestSecurityContext& context);

private:
    BrowserSessionIssuanceService& issuanceService_;
    BrowserSessionLifecycleService& lifecycleService_;
    BrowserSessionLifetimeConfiguration lifetimeConfiguration_;
};