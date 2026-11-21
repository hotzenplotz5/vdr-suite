#include "EmbeddedBackendTeletextAuthority.h"

#include "BackendAgentLifecycle.h"
#include "BackendRuntimeGeneration.h"
#include "Database.h"

EmbeddedBackendTeletextAuthority::EmbeddedBackendTeletextAuthority(
    Database& database,
    EmbeddedBackendLifecycleService& lifecycleService,
    BackendAgentLifecycleService& agentLifecycleService)
    : database_(database),
      lifecycleService_(lifecycleService),
      agentLifecycleService_(agentLifecycleService)
{
}

TeletextBackendAuthorityState
EmbeddedBackendTeletextAuthority::stateForBackend(
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
        TeletextBackendAuthorityState result;
        result.present = true;
        result.online =
            agent.state == BackendAgentConnectionState::Online;
        result.backendGeneration = agent.backendGeneration;
        return result;
    }

    const EmbeddedBackendLifecycleState state =
        lifecycleService_.statusForBackend(backendId, now);

    TeletextBackendAuthorityState result;
    result.present = state.present;
    result.online = state.online;
    result.backendGeneration = state.backendGeneration;
    return result;
}
