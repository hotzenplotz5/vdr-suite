#pragma once

#include "AddonActivationIntentRepository.h"
#include "MutationOperationRepository.h"
#include "MediaToolsImportRunLock.h"

#include <cstdint>
#include <optional>
#include <string>

namespace vdrsuite::addons {

// Global, point-in-time diagnostic. A positive result is NEVER an install
// authorization: no exclusive package transaction, provenance/rollback
// verification, or durable Agent state attestation is conferred.
enum class MaintenanceInspectionReason {
    forbidden,
    incompatibleDatabases,
    runLockUnavailable,
    storageUnavailable,
    globalFreezeRequired,
    auditedDisableMissing,
    activationStillEnabled,
    operationsUnresolved,
    agentCommandsUnresolved,
    agentResultsMissing,
    agentCapabilityStillAdvertised,
    locallyQuiescentSnapshot,
};
struct MaintenanceInspectionResult {
    bool locallyQuiescent = false;
    MaintenanceInspectionReason reason =
        MaintenanceInspectionReason::storageUnavailable;
    std::int64_t globalFreezeRevision = 0;
};

// A move-only, diagnostic lock hold. Keeps the SAME verified exclusive
// flock inode while a future trusted coordinator performs its further
// independent checks. It is NOT a maintenance permit, does not freeze
// administrators or SQLite after the snapshot, and expires on destruction.
// No API route, package action or source modification may be driven from it.
struct MaintenanceInspectionHold {
    MaintenanceInspectionResult inspection;
    std::optional<vdrsuite::mediatools::MediaToolsImportRunLock> exclusive;
    bool holdsExclusiveLock() const noexcept
    {
        return inspection.locallyQuiescent && exclusive.has_value() &&
            exclusive->ownsExclusiveLock();
    }
};

// One Core SQLite authority, one global fixed Media Tools lock, no API route,
// no package edits. A future installer must hold its own exclusive lock over
// the entire separately approved transaction, NOT use this snapshot as permit.
class AddonMediaToolsMaintenanceInspection final {
public:
    AddonMediaToolsMaintenanceInspection(
        Database& database,
        const AddonActivationIntentRepository& intents,
        const vdrsuite::operations::MutationOperationRepository& operations)
        : database_(database), intents_(intents), operations_(operations) {}

    MaintenanceInspectionResult inspect(
        const RequestSecurityContext& actor,
        const std::string& trustedRoot) const;

    // Retains the exclusive lock on a successful diagnostic, enabling a
    // separate coordination phase to prove child exclusion throughout its
    // lifetime. The SQLite freeze/Agent snapshot MUST be independently
    // revalidated; this method cannot authorize a package operation.
    MaintenanceInspectionHold inspectAndHold(
        const RequestSecurityContext& actor,
        const std::string& trustedRoot) const;
private:
    Database& database_;
    const AddonActivationIntentRepository& intents_;
    const vdrsuite::operations::MutationOperationRepository& operations_;
};

} // namespace vdrsuite::addons
