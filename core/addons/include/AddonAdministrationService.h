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
// The authenticated daemon administration HTTP adapter invokes this
// coordinator, which never parses requests, installs packages or executes
// a handler. Enable-time provider facts come from trusted Core callbacks,
// never caller-supplied optimistic evidence.
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
