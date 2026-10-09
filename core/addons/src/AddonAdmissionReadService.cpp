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

    // The global administrator role is NOT needed to perform the operation.
    // This intentionally bypasses only admin inventory visibility, never the
    // canonical per-operation grant checks above.
    const IntentResult intent = intents_.readForAdmission(
        "rectools", verified.backendId);
    if (intent.status != IntentStatus::ok)
        return {{false, AccessReason::activationStateUnavailable}, 0, false};

    verified.administratorEnabled = intent.intent.desiredEnabled;
    const AccessDecision final = canUseMediaImport(actor, backend, verified);
    return {final, intent.intent.revision, false};
}

} // namespace vdrsuite::addons
