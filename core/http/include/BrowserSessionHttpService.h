#pragma once

#include "HttpServerResponse.h"
#include "SecurityIdentity.h"

class BrowserSessionIssuanceService;
class BrowserSessionLifecycleService;

class BrowserSessionHttpService
{
public:
    BrowserSessionHttpService(
        BrowserSessionIssuanceService& issuanceService,
        BrowserSessionLifecycleService& lifecycleService);

    HttpServerResponse login(
        const RequestSecurityContext& context);
    HttpServerResponse logout(
        const RequestSecurityContext& context);

private:
    BrowserSessionIssuanceService& issuanceService_;
    BrowserSessionLifecycleService& lifecycleService_;
};
