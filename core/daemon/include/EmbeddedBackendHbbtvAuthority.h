#pragma once

#include "EmbeddedBackendLifecycle.h"
#include "HbbtvControlPlaneReadService.h"

class EmbeddedBackendHbbtvAuthority final :
    public IHbbtvBackendAuthority
{
public:
    explicit EmbeddedBackendHbbtvAuthority(
        EmbeddedBackendLifecycleService& lifecycleService);

    HbbtvBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const override;

private:
    EmbeddedBackendLifecycleService& lifecycleService_;
};
