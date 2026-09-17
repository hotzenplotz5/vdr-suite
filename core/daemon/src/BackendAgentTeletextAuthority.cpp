#include "BackendAgentTeletextAuthority.h"

BackendAgentTeletextAuthority::BackendAgentTeletextAuthority(
    BackendAgentLifecycleService& lifecycleService)
    : lifecycleService_(lifecycleService)
{
}

TeletextBackendAuthorityState BackendAgentTeletextAuthority::stateForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    const BackendAgentStatus status =
        lifecycleService_.statusForBackend(backendId, now);

    TeletextBackendAuthorityState result;
    result.present = status.present;
    result.online =
        status.state == BackendAgentConnectionState::Online;
    result.backendGeneration = status.backendGeneration;
    return result;
}
