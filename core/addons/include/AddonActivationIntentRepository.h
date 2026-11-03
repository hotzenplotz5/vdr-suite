#pragma once

#include "AddonAccessPolicy.h"
#include "Database.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace vdrsuite::addons {

// A persisted administrator preference is NOT a runtime activation grant.
enum class IntentStatus {
    ok,
    invalid,
    forbidden,
    ineligible,
    revisionConflict,
    unavailable
};

struct ActivationIntent {
    std::string moduleId;
    std::string backendId;
    bool desiredEnabled = false;
    std::int64_t revision = 0; // absent == 0, first explicit update == 1
    // Zero/empty for disabled or legacy/unbound intents. The saved Agent
    // identity and reviewed digest identify the administrator-approved provider.
    // A live Agent generation/instance is checked independently at each
    // capability/operation fence, not treated as a permanent admin preference.
    std::uint64_t backendGeneration = 0;
    std::string agentId;
    std::string agentInstanceId;
    std::string approvedHandlerSha256;
};

// An explicitly administrator-approved, initially installed Media Tools
// handler is on by default once Core verifies the independent backend-specific
// approval AND the current authenticated Agent/handler. The installer does NOT
// bypass this provider check. Revision zero is the not-yet-overridden state.
// An explicit administrator disable creates revision >= 1 and always wins.
// A later enable pins the enrolled Agent identity and reviewed handler digest.
// Current live Agent lease, process generation, capabilities, backend authority,
// actor grants and operation receipts are independently verified by the fences.
// Losing a lease pauses operations; a verified restart of the SAME enrollment
// and SAME handler does not rewrite or revoke administrator intent.
inline bool mediaToolsActivationPermits(
    const ActivationIntent& intent,
    const std::string& agentId,
    const std::string& instanceId,
    std::uint64_t generation,
    const std::string& trustedHandlerSha256)
{
    if (intent.moduleId != "rectools" || agentId.empty() ||
        instanceId.empty() || generation == 0 ||
        trustedHandlerSha256.size() != 64)
        return false;
    if (intent.revision == 0) {
        // No administrator override exists yet. This path is reached only
        // through the Core-verified provider and independent root approval
        // lookup; it must not be treated as a standalone trust proof.
        // A forged/partial legacy intent must not inherit default enable.
        return !intent.desiredEnabled && intent.backendGeneration == 0 &&
            intent.agentId.empty() && intent.agentInstanceId.empty() &&
            intent.approvedHandlerSha256.empty();
    }
    // The administrator's explicit enable is persistent configuration, not
    // an Agent lease. An enable saved while the Agent was offline has no
    // binding; executing still independently requires an authenticated
    // current Agent, valid command capability and Core-verified root approval.
    if (!intent.desiredEnabled) return false;
    if (intent.backendGeneration == 0 && intent.agentId.empty() &&
        intent.agentInstanceId.empty() &&
        intent.approvedHandlerSha256.empty()) return true;
    // Existing pinned approvals retain their exact enrolled Agent/handler.
    return intent.backendGeneration > 0 &&
        !intent.agentInstanceId.empty() &&
        intent.agentId == agentId &&
        intent.approvedHandlerSha256 == trustedHandlerSha256;
}

struct IntentResult {
    IntentStatus status = IntentStatus::unavailable;
    ActivationIntent intent;
};

struct IntentCatalogResult {
    IntentStatus status = IntentStatus::unavailable;
    std::vector<ActivationIntent> intents;
};

struct IntentMutation {
    std::string moduleId;
    std::string backendId;
    bool desiredEnabled = false;
    std::int64_t expectedRevision = -1;
};

// Core-internal, synchronous, isolated SQLite desired-state owner.
// No code loading, job dispatch or package installations. The authenticated
// daemon administration facade calls this repository through audited CAS,
// with current Core-derived trusted provider evidence.
// Caller-supplied runtime evidence is never activation authority.
class AddonActivationIntentRepository {
public:
    explicit AddonActivationIntentRepository(Database& database)
        : database_(database) {}

    bool ensureSchema();
    const Database* databaseIdentity() const noexcept { return &database_; }
    IntentResult read(const RequestSecurityContext& actor,
                      const std::string& moduleId,
                      const std::string& backendId) const;
    IntentCatalogResult readCatalog(const RequestSecurityContext& actor,
                                    const std::string& backendId) const;

    IntentResult update(const RequestSecurityContext& actor,
                        const IntentMutation& mutation,
                        const BackendAccessDecision& backend,
                        const RuntimeEvidence& evidence);

    // Future management routes MUST use this path with the existing
    // AccountabilityEventRepository on the same Database transaction.
    // Failure to append audit aborts the state change, including its CAS.
    IntentResult updateAudited(const RequestSecurityContext& actor,
                               const IntentMutation& mutation,
                               const BackendAccessDecision& backend,
                               const RuntimeEvidence& evidence,
                               const std::function<bool()>& appendAudit);

private:
    // Only the internal dry-run admission service may read desired state
    // without a global administrator context. No public route may use it.
    friend class AddonAdmissionReadService;
    // Execution fence may read desired admin intent as one necessary condition;
    // it cannot turn this intent into executable authority by itself.
    friend class MediaImportOperationFence;
    friend class MediaShrinkOperationFence;

    IntentResult readForAdmission(const std::string& moduleId,
                                  const std::string& backendId) const;

    IntentResult readInTransaction(const std::string& moduleId,
                                   const std::string& backendId) const;
    static bool validModule(const std::string& moduleId);
    static bool validBackend(const std::string& backendId);
    static bool eligibleForFutureEnable(const IntentMutation& mutation,
                                        const BackendAccessDecision& backend,
                                        const RuntimeEvidence& evidence);

    IntentResult updateInternal(const RequestSecurityContext& actor,
                                const IntentMutation& mutation,
                                const BackendAccessDecision& backend,
                                const RuntimeEvidence& evidence,
                                const std::function<bool()>* appendAudit);

    Database& database_;
};

} // namespace vdrsuite::addons
