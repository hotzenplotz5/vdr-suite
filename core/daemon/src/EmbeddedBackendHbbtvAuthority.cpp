#include "EmbeddedBackendHbbtvAuthority.h"

EmbeddedBackendHbbtvAuthority::EmbeddedBackendHbbtvAuthority(
    EmbeddedBackendLifecycleService& lifecycleService)
    : lifecycleService_(lifecycleService)
{
}

HbbtvBackendAuthorityState
EmbeddedBackendHbbtvAuthority::stateForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    const EmbeddedBackendLifecycleState state =
        lifecycleService_.statusForBackend(backendId, now);

    HbbtvBackendAuthorityState result;
    result.present = state.present;
    result.online = state.online;
    result.backendGeneration = state.backendGeneration;
    return result;
}
