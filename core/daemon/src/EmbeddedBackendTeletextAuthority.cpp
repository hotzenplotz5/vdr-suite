#include "EmbeddedBackendTeletextAuthority.h"

EmbeddedBackendTeletextAuthority::EmbeddedBackendTeletextAuthority(
    EmbeddedBackendLifecycleService& lifecycleService)
    : lifecycleService_(lifecycleService)
{
}

TeletextBackendAuthorityState
EmbeddedBackendTeletextAuthority::stateForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    const EmbeddedBackendLifecycleState state =
        lifecycleService_.statusForBackend(backendId, now);

    TeletextBackendAuthorityState result;
    result.present = state.present;
    result.online = state.online;
    result.backendGeneration = state.backendGeneration;
    return result;
}
