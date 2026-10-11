#include "PublicRecordingDevicePlaybackAdmission.h"

#include "AuthorizationService.h"

#include <utility>

PublicRecordingDevicePlaybackAdmission::PublicRecordingDevicePlaybackAdmission(
    const PublicRecordingPlaybackTargetResolver& resolver,
    BackendAvailable backendAvailable)
    : resolver_(resolver), backendAvailable_(std::move(backendAvailable)) {}

PublicRecordingDevicePlaybackAdmissionResult
PublicRecordingDevicePlaybackAdmission::admit(
    const RequestSecurityContext& context,
    const std::string& backendId,
    const std::string& publicRecordingId) const
{
    using Status = PublicRecordingDevicePlaybackAdmissionStatus;
    // Reject a browser session, Basic actor or arbitrary service principal.
    // A Device proof is a *pair* of canonical Device and device-app
    // Credential identities bound to one active Service actor.
    if (!context.authenticated())
        return {Status::unauthenticated, {}};
    if (context.actor.type != ActorType::Service ||
        !context.device || context.device->deviceId.empty() ||
        !context.credential || context.credential->credentialId.empty() ||
        context.session.has_value())
        return {Status::forbidden, {}};

    // A caller may not opt out of resolving live grants. Every playback
    // admission requires a fresh persistent grant resolution.
    if (context.permissionGrantResolution ==
        PermissionGrantResolutionState::Unavailable)
        return {Status::unavailable, {}};
    if (context.permissionGrantResolution !=
        PermissionGrantResolutionState::Resolved)
        return {Status::forbidden, {}};

    const AuthorizationDecision permission = AuthorizationService().authorize(
        context, {"media.recording.play", backendId, "media.recording.play"});
    if (!permission.allowed)
        return {Status::forbidden, {}};

    // Server-owned backend state, never a client-provided online flag.
    // This check is repeated by the future session issuer at creation time.
    if (!backendAvailable_)
        return {Status::unavailable, {}};
    const auto target = resolver_.resolve(backendId, publicRecordingId);
    if (target.status == PublicRecordingPlaybackTargetStatus::invalidRequest)
        return {Status::invalidRequest, {}};
    if (target.status != PublicRecordingPlaybackTargetStatus::ready)
        return {Status::notFound, {}};
    if (!backendAvailable_(backendId))
        return {Status::unavailable, {}};
    return {Status::ready, target.recording};
}
