#pragma once

#include "TimerIntent.h"

#include <string>
#include <vector>

class Database;

namespace vdrsuite::timers
{

enum class TimerIntentRepositoryStatus
{
    ok,
    invalid,
    notFound,
    conflict,
    alreadyExists,
    storageError,
};

struct TimerIntentRepositoryResult
{
    TimerIntentRepositoryStatus status = TimerIntentRepositoryStatus::storageError;
    TimerIntent intent;

    bool ok() const { return status == TimerIntentRepositoryStatus::ok; }
};

struct TimerIntentRepositoryListResult
{
    TimerIntentRepositoryStatus status = TimerIntentRepositoryStatus::storageError;
    std::vector<TimerIntent> intents;

    bool ok() const { return status == TimerIntentRepositoryStatus::ok; }
};

class TimerIntentRepository
{
public:
    explicit TimerIntentRepository(Database& database);

    bool ensureSchema();

    // Creation owns the initial revision token. Callers must leave
    // intentRevision empty; the durable result starts at revision "1".
    TimerIntentRepositoryResult create(const TimerIntent& intent);

    TimerIntentRepositoryResult findById(const std::string& timerIntentId);

    // Exact-equivalence lookup uses the versioned semantic identity from the
    // TimerIntent domain contract. It is evidence only; duplicate policy is
    // enforced by the service/scheduler layer in later slices.
    TimerIntentRepositoryListResult findEquivalent(const TimerIntentSpec& spec);

    // Update is optimistic-concurrency controlled. next.intentRevision must
    // equal expectedRevision, and the repository alone issues the successor
    // revision after a successful durable write.
    TimerIntentRepositoryResult update(
        const TimerIntent& next,
        const std::string& expectedRevision);

private:
    Database& database_;
};

}
