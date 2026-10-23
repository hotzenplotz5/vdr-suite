#pragma once

#include "AddonActivationIntentRepository.h"

#include <functional>
#include <optional>
#include <string>
#include <utility>

class AccountabilityEventRepository;

namespace vdrsuite::addons {

struct AdministrationAuditContext {
    std::string eventId;
    std::string occurredAt;
};

// Internal-only administrative intent coordinator. Reuses Suite's existing
// persistent activation-intent owner and append-only accountability owner.
// No HTTP entrypoint, package install or handler execution.
// Enable-time backend and provider facts must come from a trusted Core
// callback; caller-supplied optimistic evidence cannot activate a module.
class AddonAdministrationService {
public:
    struct TrustedActivationSnapshot {
        BackendAccessDecision backend;
        RuntimeEvidence provider;
    };
    using TrustedProviderRead = std::function<
        std::optional<TrustedActivationSnapshot>(
            const std::string& moduleId, const std::string& backendId)>;

    AddonAdministrationService(AddonActivationIntentRepository& intents,
                               AccountabilityEventRepository& accountability,
                               TrustedProviderRead trustedProviderRead = {})
        : intents_(intents), accountability_(accountability),
          trustedProviderRead_(std::move(trustedProviderRead)) {}

    IntentResult setDesiredState(const RequestSecurityContext& actor,
                                 const IntentMutation& mutation,
                                 const BackendAccessDecision& backend,
                                 const RuntimeEvidence& evidence,
                                 const AdministrationAuditContext& context);

private:
    AddonActivationIntentRepository& intents_;
    AccountabilityEventRepository& accountability_;
    TrustedProviderRead trustedProviderRead_;
};

} // namespace vdrsuite::addons
