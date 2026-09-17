#pragma once

#include "BackendAgentLifecycle.h"
#include "TeletextControlPlaneReadService.h"

class BackendAgentTeletextAuthority final :
    public ITeletextBackendAuthority
{
public:
    explicit BackendAgentTeletextAuthority(
        BackendAgentLifecycleService& lifecycleService);

    TeletextBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const override;

private:
    BackendAgentLifecycleService& lifecycleService_;
};
