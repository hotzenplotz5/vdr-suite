#include "AddonAccessPolicy.h"

#include <algorithm>

namespace vdrsuite::addons {
namespace {
bool hasGrant(
    const RequestSecurityContext& context,
    const std::string& permission,
    const std::string& backendId)
{
    return std::any_of(
        context.grants.begin(), context.grants.end(),
        [&](const PermissionGrant& grant) {
            return grant.permission == permission &&
                (grant.backendId == "*" || grant.backendId == backendId);
        });
}

AccessDecision deny(AccessReason reason)
{
    return {false, reason};
}
} // namespace

AccessDecision canInspectInventory(const RequestSecurityContext& actor)
{
    if (!actor.authenticated())
        return deny(AccessReason::unauthenticated);
    if (actor.permissionGrantResolution != PermissionGrantResolutionState::Resolved)
        return deny(AccessReason::grantsUnavailable);
    if (!hasGrant(actor, "role.admin", "*"))
        return deny(AccessReason::notAdministrator);
    return {true, AccessReason::allowed};
}

AccessDecision canUseMediaImport(
    const RequestSecurityContext& actor,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence)
{
    if (!actor.authenticated())
        return deny(AccessReason::unauthenticated);
    if (actor.permissionGrantResolution != PermissionGrantResolutionState::Resolved)
        return deny(AccessReason::grantsUnavailable);

    // This binding is compiled into Suite, never selected by addon.json.
    if (evidence.moduleId != "rectools" ||
        evidence.packageName != "vdr-suite-addon-media-tools")
        return deny(AccessReason::unknownModule);
    if (!evidence.installed || !evidence.manifestValid)
        return deny(AccessReason::invalidInstallation);
    if (!evidence.packageTrusted)
        return deny(AccessReason::untrustedPackage);
    if (!evidence.versionCompatible)
        return deny(AccessReason::incompatibleVersion);
    if (!evidence.administratorEnabled)
        return deny(AccessReason::disabled);
    if (!evidence.handlerRegistered || !evidence.handlerHealthy)
        return deny(AccessReason::handlerUnavailable);

    // Do not reimplement write-mode logic: reuse canonical policy evidence.
    if (evidence.backendId.empty() || evidence.backendId == "*" ||
        !backend.backendFound || !backend.allowed || backend.readOnly ||
        evidence.backendId != backend.backendId)
        return deny(AccessReason::backendUnavailable);

    // Separate import permission. Existing 'recordings.execute', generic
    // admin and media-play permissions deliberately cannot substitute.
    if (!hasGrant(actor, "addons.media.import", evidence.backendId))
        return deny(AccessReason::permissionDenied);

    return {true, AccessReason::allowed};
}

} // namespace vdrsuite::addons
