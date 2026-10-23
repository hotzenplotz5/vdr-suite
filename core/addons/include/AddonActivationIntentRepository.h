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
    // Zero/empty for disabled or legacy/unbound intents. The operation
    // fences MUST compare all three fields to a current authenticated Agent.
    std::uint64_t backendGeneration = 0;
    std::string agentId;
    std::string agentInstanceId;
};

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
