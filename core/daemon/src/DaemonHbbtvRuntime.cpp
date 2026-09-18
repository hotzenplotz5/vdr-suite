#include "DaemonHbbtvRuntime.h"

#include "BackendRuntimeContext.h"
#include "EmbeddedBackendHbbtvAuthority.h"
#include "HbbtvApiRuntime.h"
#include "HbbtvControlPlaneReadService.h"

#include <memory>
#include <string>
#include <utility>

namespace
{

std::unique_ptr<EmbeddedBackendHbbtvAuthority> hbbtvBackendAuthority;
std::unique_ptr<HbbtvControlPlaneReadService> hbbtvControlPlaneReadService;

}

bool configureDaemonHbbtvRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts)
{
    resetDaemonHbbtvRuntime();

    auto authority = std::make_unique<EmbeddedBackendHbbtvAuthority>(
        embeddedBackendLifecycleService);

    auto readService = std::make_unique<HbbtvControlPlaneReadService>(
        backendRegistryService,
        snapshotReadService,
        *authority,
        [&backendRuntimeContexts](const std::string& backendId)
            -> SuiteBridgeHbbtvResolver* {
            for (const auto& context : backendRuntimeContexts)
            {
                if (!context || context->backendId != backendId)
                {
                    continue;
                }
                return context->ensureHbbtvResolver();
            }
            return nullptr;
        });

    if (!HbbtvApiRuntime::instance().configure(*readService))
    {
        return false;
    }

    hbbtvBackendAuthority = std::move(authority);
    hbbtvControlPlaneReadService = std::move(readService);
    return true;
}

void resetDaemonHbbtvRuntime()
{
    HbbtvApiRuntime::instance().reset();
    hbbtvControlPlaneReadService.reset();
    hbbtvBackendAuthority.reset();
}

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService()
{
    return hbbtvControlPlaneReadService.get();
}
