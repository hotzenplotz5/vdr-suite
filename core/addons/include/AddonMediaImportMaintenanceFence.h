#pragma once

#include "AddonAccessPolicy.h"
#include "Database.h"
#include "MutationOperationRepository.h"

#include <cstdint>
#include <string>

class AccountabilityEventRepository;

namespace vdrsuite::addons {

// Root/admin/operator package work is NOT performed by this Core component.
// A frozen gate affects every backend in the shared MutationOperation store.
// All future media.import reservations read it in the same SQLite write
// transaction as operation creation, even across separate Suite processes.
enum class MaintenanceFenceStatus {
    ok,
    forbidden,
    invalid,
    revisionConflict,
    unavailable,
};

struct MaintenanceFenceState {
    MaintenanceFenceStatus status = MaintenanceFenceStatus::unavailable;
    bool frozen = false;
    std::int64_t revision = 0; // schema exists, absent row -> unfrozen rev 0
};

struct MaintenanceFenceAuditContext {
    std::string eventId;
    std::string occurredAt;
};

// Private Core-only administrator gate; not an API route or package hook.
// Fencing new reservations cannot stop an already executing Agent child.
// Package replacement still requires complete drain, provenance and consent.
class AddonMediaImportMaintenanceFence {
public:
    AddonMediaImportMaintenanceFence(
        Database& db,
        vdrsuite::operations::MutationOperationRepository& operations,
        AccountabilityEventRepository& audit)
        : db_(db), operations_(operations), audit_(audit) {}

    MaintenanceFenceState read(const RequestSecurityContext& actor) const;

    MaintenanceFenceState setFrozen(
        const RequestSecurityContext& actor, bool frozen,
        std::int64_t expectedRevision,
        const MaintenanceFenceAuditContext& auditContext);

private:
    Database& db_;
    vdrsuite::operations::MutationOperationRepository& operations_;
    AccountabilityEventRepository& audit_;
};

} // namespace vdrsuite::addons
