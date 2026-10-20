#pragma once

#include "AddonActivationIntentRepository.h"

#include <string>

class AccountabilityEventRepository;

namespace vdrsuite::addons {

struct AdministrationAuditContext {
    std::string eventId;
    std::string occurredAt;
};

// Internal-only administrative intent coordinator. Reuses Suite's existing
// persistent activation-intent owner and append-only accountability owner.
// No HTTP entrypoint, package install or handler execution.
class AddonAdministrationService {
public:
    AddonAdministrationService(AddonActivationIntentRepository& intents,
                               AccountabilityEventRepository& accountability)
        : intents_(intents), accountability_(accountability) {}

    IntentResult setDesiredState(const RequestSecurityContext& actor,
                                 const IntentMutation& mutation,
                                 const BackendAccessDecision& backend,
                                 const RuntimeEvidence& evidence,
                                 const AdministrationAuditContext& context);

private:
    AddonActivationIntentRepository& intents_;
    AccountabilityEventRepository& accountability_;
};

} // namespace vdrsuite::addons
