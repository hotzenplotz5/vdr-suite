#pragma once

#include "EmbeddedBackendLifecycle.h"
#include "HbbtvControlPlaneReadService.h"

class BackendAgentLifecycleService;
class Database;

class EmbeddedBackendHbbtvAuthority final :
    public IHbbtvBackendAuthority
{
public:
    EmbeddedBackendHbbtvAuthority(
        Database& database,
        EmbeddedBackendLifecycleService& lifecycleService,
        BackendAgentLifecycleService& agentLifecycleService);

    HbbtvBackendAuthorityState stateForBackend(
        const std::string& backendId,
        std::int64_t now) const override;

private:
    Database& database_;
    EmbeddedBackendLifecycleService& lifecycleService_;
    BackendAgentLifecycleService& agentLifecycleService_;
};
