#pragma once

#include "EmbeddedBackendLifecycle.h"
#include "TeletextControlPlaneReadService.h"

class BackendAgentLifecycleService;
class Database;

class EmbeddedBackendTeletextAuthority final :
    public ITeletextBackendAuthority
{
public:
    EmbeddedBackendTeletextAuthority(
        Database& database,
        EmbeddedBackendLifecycleService& lifecycleService,
        BackendAgentLifecycleService& agentLifecycleService);

    TeletextBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const override;

private:
    Database& database_;
    EmbeddedBackendLifecycleService& lifecycleService_;
    BackendAgentLifecycleService& agentLifecycleService_;
};
