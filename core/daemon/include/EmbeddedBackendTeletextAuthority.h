#pragma once

#include "EmbeddedBackendLifecycle.h"
#include "TeletextControlPlaneReadService.h"

class EmbeddedBackendTeletextAuthority final :
    public ITeletextBackendAuthority
{
public:
    explicit EmbeddedBackendTeletextAuthority(
        EmbeddedBackendLifecycleService& lifecycleService);

    TeletextBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const override;

private:
    EmbeddedBackendLifecycleService& lifecycleService_;
};
