#include "EmbeddedBackendLifecycle.h"

#include "BackendRuntimeGeneration.h"
#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>

namespace
{
constexpr std::int64_t LeaseDurationSeconds = 30;

bool safeBackendId(const std::string& value)
{
    if (value.empty() || value.size() > 128U) return false;
    return std::all_of(
        value.begin(),
        value.end(),
        [](unsigned char character) {
            return std::isalnum(character) != 0 ||
                character == '-' || character == '_' ||
                character == '.' || character == ':';
        });
}

std::string runtimeInstanceId()
{
    std::random_device random;
    std::ostringstream output;
    output << "embedded_runtime_";
    for (int index = 0; index < 12; ++index)
    {
        output << std::hex << std::setw(2) << std::setfill('0')
               << (random() & 0xffU);
    }
    return output.str();
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindInt64(sqlite3_stmt* statement, int index, std::int64_t value)
{
    return sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

bool stepDone(sqlite3_stmt* statement)
{
    return sqlite3_step(statement) == SQLITE_DONE;
}

class Transaction
{
public:
    explicit Transaction(Database& database)
        : database_(database),
          lease_(database.acquireTransactionLease()),
          active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~Transaction()
    {
        if (active_) database_.execute("ROLLBACK;");
    }

    bool active() const { return active_; }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }

private:
    Database& database_;
    Database::TransactionLease lease_;
    bool active_;
};
}

EmbeddedBackendLifecycleService::EmbeddedBackendLifecycleService(
    Database& database)
    : database_(database)
{
}

bool EmbeddedBackendLifecycleService::ensureSchema()
{
    BackendRuntimeGenerationRepository generations(database_);
    return generations.ensureSchema() &&
        database_.execute(
            "CREATE TABLE IF NOT EXISTS embedded_backend_lifecycle ("
            "backend_id TEXT PRIMARY KEY,"
            "runtime_instance_id TEXT NOT NULL,"
            "backend_generation INTEGER NOT NULL,"
            "heartbeat_sequence INTEGER NOT NULL,"
            "started_at INTEGER NOT NULL,"
            "last_heartbeat_at INTEGER NOT NULL,"
            "lease_expires_at INTEGER NOT NULL,"
            "stopped_at INTEGER NOT NULL,"
            "updated_at INTEGER NOT NULL"
            ");");
}

bool EmbeddedBackendLifecycleService::startBackend(
    const std::string& backendId,
    std::int64_t now)
{
    if (!safeBackendId(backendId) || now < 0)
        return false;

    const std::string instanceId = runtimeInstanceId();
    if (instanceId.empty()) return false;

    Transaction transaction(database_);
    if (!transaction.active()) return false;

    BackendRuntimeGenerationRepository generations(database_);
    const BackendRuntimeGenerationAllocation allocation =
        generations.allocateInCurrentTransaction(backendId, now);
    if (!allocation.accepted || allocation.generation == 0)
        return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO embedded_backend_lifecycle "
        "(backend_id,runtime_instance_id,backend_generation,heartbeat_sequence,"
        "started_at,last_heartbeat_at,lease_expires_at,stopped_at,updated_at) "
        "VALUES (?,?,?,0,?,0,0,0,?) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "runtime_instance_id=excluded.runtime_instance_id,"
        "backend_generation=excluded.backend_generation,"
        "heartbeat_sequence=0,"
        "started_at=excluded.started_at,"
        "last_heartbeat_at=0,"
        "lease_expires_at=0,"
        "stopped_at=0,"
        "updated_at=excluded.updated_at;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, instanceId) ||
        !bindInt64(
            statement,
            3,
            static_cast<std::int64_t>(allocation.generation)) ||
        !bindInt64(statement, 4, now) ||
        !bindInt64(statement, 5, now) ||
        !stepDone(statement))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return false;
    }
    sqlite3_finalize(statement);

    if (!transaction.commit()) return false;

    std::lock_guard<std::mutex> lock(mutex_);
    runtimes_[backendId] = RuntimeIdentity{
        instanceId,
        allocation.generation};
    return true;
}

bool EmbeddedBackendLifecycleService::heartbeatBackend(
    const std::string& backendId,
    bool healthy,
    std::int64_t now)
{
    if (!healthy) return true;
    if (now < 0 ||
        now > std::numeric_limits<std::int64_t>::max() - LeaseDurationSeconds)
        return false;

    RuntimeIdentity identity;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto iterator = runtimes_.find(backendId);
        if (iterator == runtimes_.end()) return false;
        identity = iterator->second;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE embedded_backend_lifecycle SET "
        "heartbeat_sequence=heartbeat_sequence+1,"
        "last_heartbeat_at=?,lease_expires_at=?,updated_at=? "
        "WHERE backend_id=? AND runtime_instance_id=? "
        "AND backend_generation=? AND stopped_at=0;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK ||
        !bindInt64(statement, 1, now) ||
        !bindInt64(statement, 2, now + LeaseDurationSeconds) ||
        !bindInt64(statement, 3, now) ||
        !bindText(statement, 4, backendId) ||
        !bindText(statement, 5, identity.instanceId) ||
        !bindInt64(
            statement,
            6,
            static_cast<std::int64_t>(identity.backendGeneration)) ||
        !stepDone(statement))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return false;
    }
    const bool changed = sqlite3_changes(database_.handle()) == 1;
    sqlite3_finalize(statement);
    return changed;
}

bool EmbeddedBackendLifecycleService::stopBackend(
    const std::string& backendId,
    std::int64_t now)
{
    RuntimeIdentity identity;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto iterator = runtimes_.find(backendId);
        if (iterator == runtimes_.end()) return true;
        identity = iterator->second;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE embedded_backend_lifecycle SET "
        "lease_expires_at=0,stopped_at=?,updated_at=? "
        "WHERE backend_id=? AND runtime_instance_id=? "
        "AND backend_generation=?;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK ||
        !bindInt64(statement, 1, now) ||
        !bindInt64(statement, 2, now) ||
        !bindText(statement, 3, backendId) ||
        !bindText(statement, 4, identity.instanceId) ||
        !bindInt64(
            statement,
            5,
            static_cast<std::int64_t>(identity.backendGeneration)) ||
        !stepDone(statement))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return false;
    }
    sqlite3_finalize(statement);

    std::lock_guard<std::mutex> lock(mutex_);
    runtimes_.erase(backendId);
    return true;
}

EmbeddedBackendLifecycleState
EmbeddedBackendLifecycleService::statusForBackend(
    const std::string& backendId,
    std::int64_t now) const
{
    EmbeddedBackendLifecycleState result;

    RuntimeIdentity identity;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        const auto iterator = runtimes_.find(backendId);
        if (iterator == runtimes_.end()) return result;
        identity = iterator->second;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT runtime_instance_id,backend_generation,heartbeat_sequence,"
        "last_heartbeat_at,lease_expires_at,stopped_at "
        "FROM embedded_backend_lifecycle WHERE backend_id=? LIMIT 1;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return result;
    }

    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* instanceText =
            sqlite3_column_text(statement, 0);
        const std::string instanceId = instanceText == nullptr
            ? std::string()
            : std::string(
                reinterpret_cast<const char*>(instanceText));
        const std::int64_t generation = sqlite3_column_int64(statement, 1);
        const std::int64_t heartbeatSequence =
            sqlite3_column_int64(statement, 2);
        const std::int64_t lastHeartbeatAt =
            sqlite3_column_int64(statement, 3);
        const std::int64_t leaseExpiresAt =
            sqlite3_column_int64(statement, 4);
        const std::int64_t stoppedAt =
            sqlite3_column_int64(statement, 5);

        if (instanceId == identity.instanceId &&
            generation > 0 &&
            static_cast<std::uint64_t>(generation) ==
                identity.backendGeneration &&
            heartbeatSequence >= 0 &&
            lastHeartbeatAt >= 0 &&
            leaseExpiresAt >= 0)
        {
            result.present = true;
            result.backendGeneration =
                static_cast<std::uint64_t>(generation);
            result.heartbeatSequence =
                static_cast<std::uint64_t>(heartbeatSequence);
            result.lastHeartbeatAt = lastHeartbeatAt;
            result.leaseExpiresAt = leaseExpiresAt;
            result.online =
                stoppedAt == 0 &&
                leaseExpiresAt > 0 &&
                now <= leaseExpiresAt;
        }
    }
    sqlite3_finalize(statement);
    return result;
}
