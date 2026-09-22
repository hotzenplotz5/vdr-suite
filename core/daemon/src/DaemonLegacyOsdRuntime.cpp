#include "DaemonLegacyOsdRuntime.h"

#include "BackendAccessPolicy.h"
#include "BackendAgentLifecycle.h"
#include "BackendAgentCommandDelivery.h"
#include "LegacyOsdInputService.h"
#include "BackendRegistryService.h"
#include "Database.h"
#include "LegacyOsdApiRuntime.h"
#include "LegacyOsdSessionService.h"
#include "OsdControllerLeaseService.h"
#include "OsdViewerBindingService.h"
#include "SecurityIdentityRepository.h"
#include "SecurityPermissionGrantRepository.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

namespace
{
std::unique_ptr<SecurityPermissionGrantRepository>
    legacyOsdPermissionGrantRepository;
std::unique_ptr<LegacyOsdSessionService> legacyOsdSessionService;
std::unique_ptr<OsdViewerBindingService> osdViewerBindingService;
std::unique_ptr<OsdControllerLeaseService> osdControllerLeaseService;
std::unique_ptr<LegacyOsdInputService> legacyOsdInputService;
BackendAgentCommandDeliveryService* legacyOsdCommandDeliveryService = nullptr;
}

bool configureDaemonLegacyOsdRuntime(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BackendAgentLifecycleService& lifecycleService,
    BackendAgentRepository& agentRepository,
    BackendAgentCommandRepository& commandRepository,
    BackendAgentCommandDeliveryService& commandDeliveryService,
    BackendRegistryService& backendRegistryService,
    BackendAccessPolicy& backendAccessPolicy)
{
    resetDaemonLegacyOsdRuntime();

    auto grants =
        std::make_unique<SecurityPermissionGrantRepository>(database);
    if (!grants->ensureSchema()) return false;
    SecurityPermissionGrantRepository* grantRepository = grants.get();

    auto contextResolver =
        [&identityRepository, grantRepository](
            const std::string& actorId, const std::string&)
            -> std::optional<RequestSecurityContext>
        {
            const auto actor = identityRepository.findActor(actorId);
            if (!actor.has_value() || !actor->active || actor->revoked)
                return std::nullopt;

            const auto resolution =
                grantRepository->findActiveGrantsForActor(actorId);
            if (!resolution.available) return std::nullopt;

            RequestSecurityContext context;
            context.authenticationState = AuthenticationState::Authenticated;
            context.actor.actorId = actor->actorId;
            context.actor.type = actor->type;
            context.actor.displayName = actor->displayName;
            context.actor.active = actor->active && !actor->revoked;
            context.grants = resolution.grants;
            context.permissionGrantResolution =
                PermissionGrantResolutionState::Resolved;
            return context;
        };

    auto sessions = std::make_unique<LegacyOsdSessionService>(
        contextResolver,
        [&lifecycleService](const std::string& backendId, std::int64_t now)
        {
            return lifecycleService.statusForBackend(backendId, now);
        },
        [&lifecycleService](
            const RequestSecurityContext& context,
            const std::string& backendId,
            std::uint64_t generation,
            std::int64_t now)
        {
            return lifecycleService.readOsdObservation(
                context, backendId, generation, now);
        });

    auto viewers = std::make_unique<OsdViewerBindingService>(*sessions);
    auto controllers = std::make_unique<OsdControllerLeaseService>(
        *sessions,
        *viewers,
        contextResolver,
        [&backendRegistryService, &backendAccessPolicy](
            const std::string& backendId)
        {
            return backendAccessPolicy.canWriteToBackend(
                backendRegistryService, backendId);
        });

    auto input = std::make_unique<LegacyOsdInputService>(
        *sessions,
        *viewers,
        *controllers,
        agentRepository,
        commandRepository);

    commandDeliveryService.setReceiptFenceCheck(
        [inputService = input.get()](
            const BackendAgentCommandReceipt& receipt,
            std::string& reasonCode)
        {
            return inputService->revalidateReceipt(receipt, reasonCode);
        });

    if (!LegacyOsdApiRuntime::instance().configure(
            *sessions, *viewers, *controllers, *input))
    {
        commandDeliveryService.setReceiptFenceCheck({});
        return false;
    }

    legacyOsdPermissionGrantRepository = std::move(grants);
    legacyOsdSessionService = std::move(sessions);
    osdViewerBindingService = std::move(viewers);
    osdControllerLeaseService = std::move(controllers);
    legacyOsdInputService = std::move(input);
    legacyOsdCommandDeliveryService = &commandDeliveryService;
    return true;
}

void resetDaemonLegacyOsdRuntime()
{
    LegacyOsdApiRuntime::instance().reset();
    if (legacyOsdCommandDeliveryService != nullptr)
        legacyOsdCommandDeliveryService->setReceiptFenceCheck({});
    legacyOsdCommandDeliveryService = nullptr;
    legacyOsdInputService.reset();
    osdControllerLeaseService.reset();
    osdViewerBindingService.reset();
    legacyOsdSessionService.reset();
    legacyOsdPermissionGrantRepository.reset();
}
