#include "AddonAdmissionReadService.h"

namespace vdrsuite::addons {

AdmissionPreview AddonAdmissionReadService::previewMediaImport(
    const RequestSecurityContext& actor,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& candidateEvidence) const
{
    // Never trust an external "administratorEnabled" flag. First check
    // independent actor, package, handler and backend evidence.
    RuntimeEvidence verified = candidateEvidence;
    verified.administratorEnabled = true;
    const AccessDecision independent = canUseMediaImport(actor, backend, verified);
    if (!independent.allowed)
        return {independent, 0, false};

    // The root-admin installation is sufficient to activate a trusted
    // handler. Ordinary actors still need the canonical scoped grant.
    const IntentResult intent = intents_.readForAdmission(
        "rectools", verified.backendId);
    if (intent.status != IntentStatus::ok)
        return {{false, AccessReason::activationStateUnavailable}, 0, false};

    const bool enabled = mediaToolsActivationPermits(
        intent.intent, verified.agentId, verified.agentInstanceId,
        verified.backendGeneration, verified.approvedHandlerSha256);
    if (!enabled && intent.intent.desiredEnabled)
        return {{false, AccessReason::activationStateUnavailable},
                intent.intent.revision, false};

    verified.administratorEnabled = enabled;
    const AccessDecision final = canUseMediaImport(actor, backend, verified);
    return {final, intent.intent.revision, false};
}

} // namespace vdrsuite::addons
