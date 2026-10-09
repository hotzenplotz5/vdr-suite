#pragma once

#include "AddonAccessPolicy.h"
#include "Database.h"

#include <cstdint>
#include <string>

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
};

struct IntentResult {
    IntentStatus status = IntentStatus::unavailable;
    ActivationIntent intent;
};

struct IntentMutation {
    std::string moduleId;
    std::string backendId;
    bool desiredEnabled = false;
    std::int64_t expectedRevision = -1;
};

// Core-internal, synchronous, isolated SQLite desired-state owner.
// No code loading, job dispatch, package installations or real activation.
// Callers must supply canonical resolved actor and backend evidence. This
// component cannot be called from public HTTP routes.
class AddonActivationIntentRepository {
public:
    explicit AddonActivationIntentRepository(Database& database)
        : database_(database) {}

    bool ensureSchema();
    IntentResult read(const RequestSecurityContext& actor,
                      const std::string& moduleId,
                      const std::string& backendId) const;
    IntentResult update(const RequestSecurityContext& actor,
                        const IntentMutation& mutation,
                        const BackendAccessDecision& backend,
                        const RuntimeEvidence& evidence);

private:
    // Only the internal dry-run admission service may read desired state
    // without a global administrator context. No public route may use it.
    friend class AddonAdmissionReadService;

    IntentResult readForAdmission(const std::string& moduleId,
                                  const std::string& backendId) const;

    IntentResult readInTransaction(const std::string& moduleId,
                                   const std::string& backendId) const;
    static bool validModule(const std::string& moduleId);
    static bool validBackend(const std::string& backendId);
    static bool eligibleForFutureEnable(const IntentMutation& mutation,
                                        const BackendAccessDecision& backend,
                                        const RuntimeEvidence& evidence);

    Database& database_;
};

} // namespace vdrsuite::addons
