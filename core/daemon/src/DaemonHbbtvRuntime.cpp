#include "DaemonHbbtvRuntime.h"

#include "AuthorizationService.h"
#include "BackendRuntimeContext.h"
#include "Database.h"
#include "EmbeddedBackendHbbtvAuthority.h"
#include "HbbtvApiRuntime.h"
#include "HbbtvApplicationSessionService.h"
#include "HbbtvControlPlaneReadService.h"
#include "SecurityConfiguration.h"
#include "SecurityPermissionGrantRepository.h"

#include <cstdlib>
#include <memory>
#include <string>
#include <utility>

namespace
{

std::unique_ptr<EmbeddedBackendHbbtvAuthority> hbbtvBackendAuthority;
std::unique_ptr<HbbtvControlPlaneReadService> hbbtvControlPlaneReadService;
std::unique_ptr<Database> hbbtvSecurityDatabase;
std::unique_ptr<SecurityPermissionGrantRepository>
    hbbtvPermissionGrantRepository;
std::unique_ptr<HbbtvApplicationSessionService>
    hbbtvApplicationSessionService;

std::string securityDatabasePath(const std::string& fallback)
{
    const char* configured =
        std::getenv("VDR_SUITE_SECURITY_DATABASE_PATH");
    if (configured != nullptr && configured[0] != '\0')
    {
        return configured;
    }
    return fallback;
}

}

bool configureDaemonHbbtvRuntime(
    const std::string& defaultDatabasePath,
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

    auto securityDatabase = std::make_unique<Database>();
    if (!securityDatabase->open(
            securityDatabasePath(defaultDatabasePath)))
    {
        return false;
    }

    auto grantRepository =
        std::make_unique<SecurityPermissionGrantRepository>(
            *securityDatabase);
    if (!grantRepository->ensureSchema())
    {
        return false;
    }

    SecurityPermissionGrantRepository* grants = grantRepository.get();
    const SecurityConfiguration securityConfiguration =
        SecurityConfiguration::fromEnvironment();

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
            [grants, securityConfiguration](
                const std::string& permission,
                const std::string& actorId,
                const std::string& backendId) {
                if (grants == nullptr)
                {
                    return false;
                }

                const SecurityPermissionGrantResolution resolution =
                    grants->findActiveGrantsForActor(actorId);

                std::vector<PermissionGrant> effectiveGrants;
                if (resolution.available)
                {
                    effectiveGrants = resolution.grants;
                }

                bool grantSourceAvailable = resolution.available;

                if (securityConfiguration.mode ==
                        SecurityMode::LegacyBasicCompatibility &&
                    !securityConfiguration.expectedAuthorizationHeader.empty() &&
                    actorId == securityConfiguration.actorId)
                {
                    effectiveGrants.insert(
                        effectiveGrants.end(),
                        securityConfiguration.grants.begin(),
                        securityConfiguration.grants.end());
                    grantSourceAvailable = true;
                }

                if (securityConfiguration.managedBasic.hasAnyConfiguration() &&
                    actorId == securityConfiguration.managedBasic.actorId)
                {
                    effectiveGrants.insert(
                        effectiveGrants.end(),
                        securityConfiguration.managedBasic.grants.begin(),
                        securityConfiguration.managedBasic.grants.end());
                    grantSourceAvailable = true;
                }

                if (!grantSourceAvailable)
                {
                    return false;
                }

                RequestSecurityContext context;
                context.authenticationState =
                    AuthenticationState::Authenticated;
                context.actor.actorId = actorId;
                context.actor.type = ActorType::User;
                context.actor.active = true;
                context.grants = std::move(effectiveGrants);
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
            *sessionService,
            [&backendRuntimeContexts](const std::string& backendId)
                -> IHbbtvPresentationSource* {
                for (const auto& context : backendRuntimeContexts)
                {
                    if (!context || context->backendId != backendId)
                        continue;
                    return context->ensureHbbtvPresentationResolver();
                }
                return nullptr;
            },
            [&backendRuntimeContexts](const std::string& backendId)
                -> IHbbtvMediaSourceResolver* {
                for (const auto& context : backendRuntimeContexts)
                {
                    if (!context || context->backendId != backendId)
                        continue;
                    return context->ensureHbbtvMediaResolver();
                }
                return nullptr;
            }))
    {
        return false;
    }

    hbbtvBackendAuthority = std::move(authority);
    hbbtvControlPlaneReadService = std::move(readService);
    hbbtvSecurityDatabase = std::move(securityDatabase);
    hbbtvPermissionGrantRepository = std::move(grantRepository);
    hbbtvApplicationSessionService = std::move(sessionService);
    return true;
}

void resetDaemonHbbtvRuntime()
{
    HbbtvApiRuntime::instance().reset();
    hbbtvApplicationSessionService.reset();
    hbbtvPermissionGrantRepository.reset();
    hbbtvSecurityDatabase.reset();
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
