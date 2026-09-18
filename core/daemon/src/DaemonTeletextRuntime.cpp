#include "DaemonTeletextRuntime.h"

#include "EmbeddedBackendTeletextAuthority.h"
#include "BackendRuntimeContext.h"
#include "TeletextApiRuntime.h"
#include "TeletextControlPlaneReadService.h"

#include <memory>
#include <string>
#include <utility>

namespace
{

std::unique_ptr<EmbeddedBackendTeletextAuthority> teletextBackendAuthority;
std::unique_ptr<TeletextControlPlaneReadService> teletextControlPlaneReadService;

}

bool configureDaemonTeletextRuntime(
    BackendRegistryService& backendRegistryService,
    VdrSnapshotReadService& snapshotReadService,
    EmbeddedBackendLifecycleService& embeddedBackendLifecycleService,
    std::vector<std::unique_ptr<BackendRuntimeContext>>& backendRuntimeContexts)
{
    resetDaemonTeletextRuntime();

    auto authority = std::make_unique<EmbeddedBackendTeletextAuthority>(
        embeddedBackendLifecycleService);

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

    if (!TeletextApiRuntime::instance().configure(*readService))
    {
        return false;
    }

    teletextBackendAuthority = std::move(authority);
    teletextControlPlaneReadService = std::move(readService);
    return true;
}

void resetDaemonTeletextRuntime()
{
    TeletextApiRuntime::instance().reset();
    teletextControlPlaneReadService.reset();
    teletextBackendAuthority.reset();
}

TeletextControlPlaneReadService* daemonTeletextControlPlaneReadService()
{
    return teletextControlPlaneReadService.get();
}
