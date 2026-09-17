#include "DaemonTeletextRuntime.h"

#include "BackendAgentTeletextAuthority.h"
#include "BackendRuntimeContext.h"
#include "TeletextControlPlaneReadService.h"

#include <memory>
#include <string>
#include <utility>

namespace
{

std::unique_ptr<BackendAgentTeletextAuthority> teletextBackendAuthority;
std::unique_ptr<TeletextControlPlaneReadService> teletextControlPlaneReadService;

}

bool configureDaemonTeletextRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    BackendAgentLifecycleService& backendAgentLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts)
{
    resetDaemonTeletextRuntime();

    auto authority = std::make_unique<BackendAgentTeletextAuthority>(
        backendAgentLifecycleService);

    auto readService = std::make_unique<TeletextControlPlaneReadService>(
        backendRegistryService,
        snapshotReadService,
        *authority,
        [&backendRuntimeContexts](const std::string& backendId)
            -> SuiteBridgeTeletextResolver* {
            for (const auto& context : backendRuntimeContexts)
            {
                if (!context || context->backendId != backendId)
                {
                    continue;
                }
                return context->ensureTeletextResolver();
            }
            return nullptr;
        });

    teletextBackendAuthority = std::move(authority);
    teletextControlPlaneReadService = std::move(readService);
    return true;
}

void resetDaemonTeletextRuntime()
{
    teletextControlPlaneReadService.reset();
    teletextBackendAuthority.reset();
}

TeletextControlPlaneReadService* daemonTeletextControlPlaneReadService()
{
    return teletextControlPlaneReadService.get();
}
