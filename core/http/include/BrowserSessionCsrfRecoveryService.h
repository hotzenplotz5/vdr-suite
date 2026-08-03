#pragma once

#include "HttpServerResponse.h"
#include "SecurityIdentity.h"

class BrowserSessionCredentialRepository;

class BrowserSessionCsrfRecoveryService
{
public:
    explicit BrowserSessionCsrfRecoveryService(
        const BrowserSessionCredentialRepository& repository);

    HttpServerResponse recover(
        const RequestSecurityContext& context) const;

private:
    const BrowserSessionCredentialRepository& repository_;
};
