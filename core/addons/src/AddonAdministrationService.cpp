#include "AddonAdministrationService.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"

#include <algorithm>
#include <cctype>

namespace vdrsuite::addons {
namespace {
bool safeAuditIdentifier(const std::string& value)
{
    return !value.empty() && value.size() <= 128 &&
        std::all_of(value.begin(), value.end(), [](unsigned char c) {
            return std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == ':';
        });
}
}

IntentResult AddonAdministrationService::setDesiredState(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence,
    const AdministrationAuditContext& context)
{
    if (actor.actor.type != ActorType::User ||
        !canInspectInventory(actor).allowed ||
        !safeAuditIdentifier(context.eventId) ||
        !safeAuditIdentifier(context.requestId) ||
        !safeAuditIdentifier(context.correlationId) ||
        context.occurredAt.empty() || context.occurredAt.size() > 64)
    {
        return {IntentStatus::forbidden,
                {mutation.moduleId, mutation.backendId, false, 0}};
    }

    AccountabilityEvent event;
    event.eventId = context.eventId;
    event.classes = "security,administration";
    event.eventType = "addon.activation-intent";
    event.severity = "info";
    event.occurredAt = context.occurredAt;
    event.actorId = actor.actor.actorId;
    event.actorType = actorTypeName(actor.actor.type);
    event.deviceId = actor.device ? actor.device->deviceId : "";
    event.sessionId = actor.session ? actor.session->sessionId : "";
    event.authenticationState = authenticationStateName(actor.authenticationState);
    event.permission = "role.admin";
    event.backendId = mutation.backendId;
    event.requestId = context.requestId;
    event.correlationId = context.correlationId;
    event.action = mutation.desiredEnabled ? "addons.intent.enable" : "addons.intent.disable";
    event.decision = "allowed";
    event.reasonCode = "administrator_intent";
    event.outcome = "committed";

    // append() is the existing Core accountability repository, invoked
    // inside the same SQLite transaction held by updateAudited(). The owning
    // runtime must construct both repositories with the SAME Database.
    return intents_.updateAudited(actor, mutation, backend, evidence,
        [&] { return accountability_.append(event); });
}

} // namespace vdrsuite::addons
