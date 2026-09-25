#include "EmbeddedBackendHbbtvAuthority.h"

#include "BackendAgentLifecycle.h"
#include "BackendRuntimeGeneration.h"
#include "Database.h"

EmbeddedBackendHbbtvAuthority::EmbeddedBackendHbbtvAuthority(
    Database& database,
    EmbeddedBackendLifecycleService& lifecycleService,
    BackendAgentLifecycleService& agentLifecycleService)
    : database_(database),
      lifecycleService_(lifecycleService),
      agentLifecycleService_(agentLifecycleService)
{
}

HbbtvBackendAuthorityState
EmbeddedBackendHbbtvAuthority::stateForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    BackendRuntimeGenerationRepository generations(database_);
    const std::uint64_t latestGeneration =
        generations.latestGeneration(backendId);

    const BackendAgentStatus agent =
        agentLifecycleService_.statusForBackend(backendId, now);
    if (agent.present &&
        agent.backendGeneration == latestGeneration)
    {
        HbbtvBackendAuthorityState result;
        result.present = true;
        result.online =
            agent.state == BackendAgentConnectionState::Online;
        result.backendGeneration = agent.backendGeneration;
        return result;
    }

    const EmbeddedBackendLifecycleState state =
        lifecycleService_.statusForBackend(backendId, now);

    HbbtvBackendAuthorityState result;
    result.present = state.present;
    result.online = state.online;
    result.backendGeneration = state.backendGeneration;
    return result;
}
