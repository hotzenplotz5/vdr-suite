#include "SearchTimerCreateDeviceAdmissionService.h"

#include "AuthorizationService.h"
#include "BackendAccessPolicy.h"
#include "BackendRegistryService.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>

namespace
{
SearchTimerCreateDeviceAdmissionResult result(
    SearchTimerCreateDeviceAdmissionStatus status,
    const vdrsuite::operations::MutationOperation& operation = {})
{
    SearchTimerCreateDeviceAdmissionResult out;
    out.status = status;
    out.operation = operation;
    return out;
}

bool exactDeviceIdentity(const RequestSecurityContext& context)
{
    return context.authenticated() &&
        context.actor.type == ActorType::Service &&
        context.device.has_value() &&
        !context.device->deviceId.empty() &&
        context.credential.has_value() &&
        !context.credential->credentialId.empty() &&
        !context.session.has_value() &&
        context.permissionGrantResolution ==
            PermissionGrantResolutionState::Resolved;
}

bool explicitCreateGrant(const RequestSecurityContext& context,
    const std::string& backendId)
{
    // AuthorizationService also supports admin and wildcard grants for
    // human actors. Neither is sufficient for a Device CREATE.
    return std::any_of(context.grants.begin(), context.grants.end(),
        [&backendId](const PermissionGrant& grant) {
            return grant.permission == "searchtimers.create" &&
                grant.backendId == backendId;
        });
}
}

SearchTimerCreateDeviceAdmissionService::
SearchTimerCreateDeviceAdmissionService(
    SearchTimerCreateAdmissionService& admission,
    const AuthorizationService& authorization,
    const BackendAccessPolicy& access,
    const BackendRegistryService& backends,
    const ISearchTimerBackendGenerationAuthority& generations)
    : admission_(admission), authorization_(authorization),
      access_(access), backends_(backends), generations_(generations)
{
}

SearchTimerCreateDeviceAdmissionResult
SearchTimerCreateDeviceAdmissionService::admit(
    const SearchTimerCreateDeviceAdmissionRequest& request) const
{
    if (!exactDeviceIdentity(request.context))
        return result(SearchTimerCreateDeviceAdmissionStatus::deniedIdentity);

    if (!explicitCreateGrant(request.context, request.backendId))
        return result(SearchTimerCreateDeviceAdmissionStatus::deniedPermission);

    AuthorizationRequest authorization;
    authorization.permission = "searchtimers.create";
    authorization.backendId = request.backendId;
    authorization.action = "searchtimers.create";
    if (!authorization_.authorize(request.context, authorization).allowed)
        return result(SearchTimerCreateDeviceAdmissionStatus::deniedPermission);

    const auto write = access_.canWriteToBackend(backends_, request.backendId);
    const auto backend = backends_.getBackend(request.backendId);
    if (!write.allowed || !backend.has_value() || !backend->online)
        return result(SearchTimerCreateDeviceAdmissionStatus::deniedBackend);

    const std::optional<std::uint64_t> generation =
        generations_.currentGeneration(request.backendId);
    if (!generation.has_value() || *generation == 0U ||
        *generation > static_cast<std::uint64_t>(
            std::numeric_limits<std::int64_t>::max()))
        return result(
            SearchTimerCreateDeviceAdmissionStatus::generationUnavailable);

    SearchTimerCreateAdmissionRequest prepared;
    prepared.actorId = request.context.actor.actorId;
    prepared.backendId = request.backendId;
    prepared.backendGeneration = *generation;
    prepared.idempotencyKey = request.idempotencyKey;
    prepared.requestedAt = request.requestedAt;
    prepared.payload = request.payload;

    const auto admission = admission_.admit(prepared);
    switch (admission.status)
    {
        case SearchTimerCreateAdmissionStatus::prepared:
            return result(SearchTimerCreateDeviceAdmissionStatus::prepared,
                admission.operation);
        case SearchTimerCreateAdmissionStatus::replayed:
            return result(SearchTimerCreateDeviceAdmissionStatus::replayed,
                admission.operation);
        case SearchTimerCreateAdmissionStatus::invalid:
            return result(SearchTimerCreateDeviceAdmissionStatus::invalid);
        case SearchTimerCreateAdmissionStatus::idempotencyConflict:
        case SearchTimerCreateAdmissionStatus::operationConflict:
            return result(SearchTimerCreateDeviceAdmissionStatus::conflict,
                admission.operation);
        case SearchTimerCreateAdmissionStatus::identityUnavailable:
        case SearchTimerCreateAdmissionStatus::repositoryUnavailable:
            return result(SearchTimerCreateDeviceAdmissionStatus::unavailable);
    }
    return result(SearchTimerCreateDeviceAdmissionStatus::unavailable);
}
