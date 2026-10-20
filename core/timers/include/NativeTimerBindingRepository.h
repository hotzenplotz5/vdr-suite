#pragma once

#include "NativeTimerBinding.h"

#include <string>
#include <vector>

class Database;

namespace vdrsuite::timers
{

enum class NativeTimerBindingRepositoryStatus
{
    ok,
    invalid,
    notFound,
    conflict,
    alreadyExists,
    nativeIdentityConflict,
    assignmentBindingConflict,
    immutableConflict,
    generationConflict,
    observationConflict,
    storageError,
};

struct NativeTimerBindingRepositoryResult
{
    NativeTimerBindingRepositoryStatus status =
        NativeTimerBindingRepositoryStatus::storageError;
    NativeTimerBinding binding;

    bool ok() const
    {
        return status == NativeTimerBindingRepositoryStatus::ok;
    }
};

struct NativeTimerBindingRepositoryListResult
{
    NativeTimerBindingRepositoryStatus status =
        NativeTimerBindingRepositoryStatus::storageError;
    std::vector<NativeTimerBinding> bindings;

    bool ok() const
    {
        return status == NativeTimerBindingRepositoryStatus::ok;
    }
};

class NativeTimerBindingRepository
{
public:
    explicit NativeTimerBindingRepository(Database& database);

    bool ensureSchema();

    // Creation owns the initial binding revision. Callers must leave
    // bindingRevision empty; a successful durable create starts at revision "1".
    NativeTimerBindingRepositoryResult create(
        const NativeTimerBinding& binding);

    NativeTimerBindingRepositoryResult findById(
        const std::string& nativeTimerBindingId);

    // backendNativeTimerId is resolved only inside its backend scope.
    // backendGeneration is observation evidence and deliberately not part of
    // the stable native-identity lookup key.
    NativeTimerBindingRepositoryResult findByBackendNativeTimer(
        const std::string& backendId,
        const std::string& backendNativeTimerId);

    NativeTimerBindingRepositoryListResult listForAssignment(
        const std::string& timerAssignmentId);

    // Keyset pagination bounds passive refresh work per poll. Only stable,
    // idle, managed/adopted bindings are candidates; this is not adoption.
    NativeTimerBindingRepositoryListResult listManagedForFreshness(
        const std::string& afterId, std::int64_t staleBefore,
        std::size_t limit);

    // Generic update is observation/revision persistence only. Stable binding
    // identity, backend/native identity, ownership and assignment relationship
    // are immutable here. Adoption/reclassification requires a later explicit
    // authorized operation.
    NativeTimerBindingRepositoryResult update(
        const NativeTimerBinding& next,
        const std::string& expectedRevision,
        bool passiveFreshnessRefresh = false);

private:
    Database& database_;
};

}
