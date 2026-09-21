#include "DaemonLegacyOsdRuntime.h"

#include "BackendAgentLifecycle.h"
#include "Database.h"
#include "LegacyOsdApiRuntime.h"
#include "LegacyOsdSessionService.h"
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
}

bool configureDaemonLegacyOsdRuntime(
    Database& database,
    SecurityIdentityRepository& identityRepository,
    BackendAgentLifecycleService& lifecycleService)
{
    resetDaemonLegacyOsdRuntime();

    auto grants =
        std::make_unique<SecurityPermissionGrantRepository>(database);
    if (!grants->ensureSchema()) return false;
    SecurityPermissionGrantRepository* grantRepository = grants.get();

    auto sessions = std::make_unique<LegacyOsdSessionService>(
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
        },
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

    if (!LegacyOsdApiRuntime::instance().configure(*sessions, *viewers))
        return false;

    legacyOsdPermissionGrantRepository = std::move(grants);
    legacyOsdSessionService = std::move(sessions);
    osdViewerBindingService = std::move(viewers);
    return true;
}

void resetDaemonLegacyOsdRuntime()
{
    LegacyOsdApiRuntime::instance().reset();
    osdViewerBindingService.reset();
    legacyOsdSessionService.reset();
    legacyOsdPermissionGrantRepository.reset();
}
