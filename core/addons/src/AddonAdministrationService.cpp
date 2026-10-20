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
bool validUtcTimestamp(const std::string& t)
{
    if (t.size() != 20 || t[4] != '-' || t[7] != '-' ||
        t[10] != 'T' || t[13] != ':' || t[16] != ':' || t[19] != 'Z')
        return false;
    for (std::size_t i = 0; i < t.size(); ++i)
    {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19)
            continue;
        if (t[i] < '0' || t[i] > '9') return false;
    }
    return true;
}

IntentResult AddonAdministrationService::setDesiredState(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence,
    const AdministrationAuditContext& context)
{
    if (intents_.databaseIdentity() != accountability_.databaseIdentity() ||
        actor.actor.type != ActorType::User ||
        !canInspectInventory(actor).allowed ||
        !safeAuditIdentifier(context.eventId) ||
        !safeAuditIdentifier(actor.requestId) ||
        !safeAuditIdentifier(actor.correlationId) ||
        !validUtcTimestamp(context.occurredAt))
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
    event.requestId = actor.requestId;
    event.correlationId = actor.correlationId;
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
