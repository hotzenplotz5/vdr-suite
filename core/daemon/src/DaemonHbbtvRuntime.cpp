#include "DaemonHbbtvRuntime.h"

#include "AuthorizationService.h"
#include "BackendRuntimeContext.h"
#include "Database.h"
#include "EmbeddedBackendHbbtvAuthority.h"
#include "HbbtvApiRuntime.h"
#include "HbbtvApplicationSessionService.h"
#include "HbbtvControlPlaneReadService.h"
#include "SecurityPermissionGrantRepository.h"

#include <memory>
#include <string>
#include <utility>

namespace
{

std::unique_ptr<EmbeddedBackendHbbtvAuthority> hbbtvBackendAuthority;
std::unique_ptr<HbbtvControlPlaneReadService> hbbtvControlPlaneReadService;
std::unique_ptr<SecurityPermissionGrantRepository>
    hbbtvPermissionGrantRepository;
std::unique_ptr<HbbtvApplicationSessionService>
    hbbtvApplicationSessionService;

}

bool configureDaemonHbbtvRuntime(
    Database& database,
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

    auto grantRepository =
        std::make_unique<SecurityPermissionGrantRepository>(database);
    if (!grantRepository->ensureSchema())
    {
        return false;
    }

    SecurityPermissionGrantRepository* grants = grantRepository.get();

    auto sessionService =
        std::make_unique<HbbtvApplicationSessionService>(
            *readService,
            [&backendRuntimeContexts](const std::string& backendId)
                -> IHbbtvRuntimeControl* {
                for (const auto& context : backendRuntimeContexts)
                {
                    if (!context || context->backendId != backendId)
                    {
                        continue;
                    }
                    return context->ensureHbbtvRuntimeResolver();
                }
                return nullptr;
            },
            [grants](
                const std::string& permission,
                const std::string& actorId,
                const std::string& backendId) {
                if (grants == nullptr)
                {
                    return false;
                }

                const SecurityPermissionGrantResolution resolution =
                    grants->findActiveGrantsForActor(actorId);
                if (!resolution.available)
                {
                    return false;
                }

                RequestSecurityContext context;
                context.authenticationState =
                    AuthenticationState::Authenticated;
                context.actor.actorId = actorId;
                context.actor.type = ActorType::User;
                context.actor.active = true;
                context.grants = resolution.grants;
                context.permissionGrantResolution =
                    PermissionGrantResolutionState::Resolved;

                AuthorizationRequest request;
                request.permission = permission;
                request.backendId = backendId;
                request.action = permission;

                return AuthorizationService().authorize(
                    context,
                    request).allowed;
            });

    if (!HbbtvApiRuntime::instance().configure(
            *readService,
            *sessionService))
    {
        return false;
    }

    hbbtvBackendAuthority = std::move(authority);
    hbbtvControlPlaneReadService = std::move(readService);
    hbbtvPermissionGrantRepository = std::move(grantRepository);
    hbbtvApplicationSessionService = std::move(sessionService);
    return true;
}

void resetDaemonHbbtvRuntime()
{
    HbbtvApiRuntime::instance().reset();
    hbbtvApplicationSessionService.reset();
    hbbtvPermissionGrantRepository.reset();
    hbbtvControlPlaneReadService.reset();
    hbbtvBackendAuthority.reset();
}

HbbtvControlPlaneReadService* daemonHbbtvControlPlaneReadService()
{
    return hbbtvControlPlaneReadService.get();
}

HbbtvApplicationSessionService* daemonHbbtvApplicationSessionService()
{
    return hbbtvApplicationSessionService.get();
}
