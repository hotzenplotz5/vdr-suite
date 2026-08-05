#include "BackendAgentLifecycle.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <limits>
#include <set>
#include <string>

namespace
{
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

bool fitsDatabaseInteger(std::uint64_t value)
{
    return value <= static_cast<std::uint64_t>(
        std::numeric_limits<std::int64_t>::max());
}

bool validAgentNumericColumns(sqlite3_stmt* statement)
{
    for (const int column : {5, 7, 10, 11})
    {
        if (sqlite3_column_int64(statement, column) < 0) return false;
    }
    return true;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(value));
}

BackendAgentRecord readAgent(sqlite3_stmt* statement)
{
    BackendAgentRecord agent;
    agent.agentId = columnText(statement, 0);
    agent.backendId = columnText(statement, 1);
    agent.actorId = columnText(statement, 2);
    agent.deviceId = columnText(statement, 3);
    agent.credentialId = columnText(statement, 4);
    agent.credentialGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 5));
    agent.agentInstanceId = columnText(statement, 6);
    agent.backendGeneration = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 7));
    agent.protocolVersion = columnText(statement, 8);
    agent.softwareVersion = columnText(statement, 9);
    agent.heartbeatSequence = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 10));
    agent.capabilityRevision = static_cast<std::uint64_t>(
        sqlite3_column_int64(statement, 11));
    agent.lastConnectedAt = sqlite3_column_int64(statement, 12);
    agent.lastHeartbeatAt = sqlite3_column_int64(statement, 13);
    agent.leaseExpiresAt = sqlite3_column_int64(statement, 14);
    agent.revoked = sqlite3_column_int64(statement, 15) != 0;
    agent.revocationReason = columnText(statement, 16);
    agent.incompatible = sqlite3_column_int(statement, 17) != 0;
    return agent;
}

constexpr const char* AgentColumns =
    "agent_id, backend_id, actor_id, device_id, credential_id, "
    "credential_generation, agent_instance_id, backend_generation, "
    "protocol_version, software_version, heartbeat_sequence, "
    "capability_revision, last_connected_at, last_heartbeat_at, "
    "lease_expires_at, revoked_at <> 0, revocation_reason, incompatible";

class Transaction
{
public:
    explicit Transaction(Database& database)
        : database_(database),
          lease_(database.acquireTransactionLease()),
          active_(database.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~Transaction()
    {
        if (active_)
        {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const { return active_; }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;"))
        {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    Database::TransactionLease lease_;
    bool active_;
};

bool executeStatement(sqlite3_stmt* statement)
{
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}
}

BackendAgentRepository::BackendAgentRepository(Database& database)
    : database_(database)
{
}

bool BackendAgentRepository::ensureSchema()
{
    return database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_enrollments ("
               "enrollment_id TEXT PRIMARY KEY,"
               "backend_id TEXT NOT NULL,"
               "token_hash TEXT NOT NULL,"
               "status TEXT NOT NULL DEFAULT 'pending',"
               "expires_at INTEGER NOT NULL,"
               "agent_id TEXT NOT NULL DEFAULT '',"
               "created_at INTEGER NOT NULL,"
               "consumed_at INTEGER NOT NULL DEFAULT 0,"
               "revoked_at INTEGER NOT NULL DEFAULT 0,"
               "revocation_reason TEXT NOT NULL DEFAULT '',"
               "CHECK(status IN ('pending','consumed','revoked','expired'))"
               ");") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_backend_agent_enrollment_backend "
               "ON backend_agent_enrollments(backend_id, status, expires_at);") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agents ("
               "agent_id TEXT PRIMARY KEY,"
               "backend_id TEXT NOT NULL,"
               "actor_id TEXT NOT NULL UNIQUE,"
               "device_id TEXT NOT NULL UNIQUE,"
               "credential_id TEXT NOT NULL UNIQUE,"
               "credential_generation INTEGER NOT NULL DEFAULT 1,"
               "agent_instance_id TEXT NOT NULL DEFAULT '',"
               "backend_generation INTEGER NOT NULL DEFAULT 0,"
               "protocol_version TEXT NOT NULL DEFAULT '',"
               "software_version TEXT NOT NULL DEFAULT '',"
               "heartbeat_sequence INTEGER NOT NULL DEFAULT 0,"
               "capability_revision INTEGER NOT NULL DEFAULT 0,"
               "last_connected_at INTEGER NOT NULL DEFAULT 0,"
               "last_heartbeat_at INTEGER NOT NULL DEFAULT 0,"
               "lease_expires_at INTEGER NOT NULL DEFAULT 0,"
               "revoked_at INTEGER NOT NULL DEFAULT 0,"
               "revocation_reason TEXT NOT NULL DEFAULT '',"
               "incompatible INTEGER NOT NULL DEFAULT 0,"
               "created_at INTEGER NOT NULL,"
               "updated_at INTEGER NOT NULL"
               ");") &&
        database_.execute(
               "CREATE UNIQUE INDEX IF NOT EXISTS idx_backend_agents_active_backend "
               "ON backend_agents(backend_id) WHERE revoked_at = 0;") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_backend_agents_backend_generation "
               "ON backend_agents(backend_id, backend_generation);") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_credential_rotations ("
               "rotation_id TEXT PRIMARY KEY,"
               "agent_id TEXT NOT NULL,"
               "from_generation INTEGER NOT NULL,"
               "to_generation INTEGER NOT NULL,"
               "rotated_at INTEGER NOT NULL,"
               "UNIQUE(agent_id, to_generation),"
               "FOREIGN KEY(agent_id) REFERENCES backend_agents(agent_id)"
               ");") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_capabilities ("
               "agent_id TEXT NOT NULL,"
               "capability_revision INTEGER NOT NULL,"
               "capability_kind TEXT NOT NULL,"
               "capability_name TEXT NOT NULL,"
               "capability_value TEXT NOT NULL,"
               "PRIMARY KEY(agent_id, capability_kind, capability_name),"
               "FOREIGN KEY(agent_id) REFERENCES backend_agents(agent_id)"
               ");") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_observation_receipts ("
               "receipt_id INTEGER PRIMARY KEY AUTOINCREMENT,"
               "backend_id TEXT NOT NULL,"
               "observation_domain TEXT NOT NULL,"
               "agent_id TEXT NOT NULL,"
               "agent_instance_id TEXT NOT NULL,"
               "backend_generation INTEGER NOT NULL,"
               "snapshot_generation INTEGER NOT NULL,"
               "producer_sequence INTEGER NOT NULL,"
               "kind TEXT NOT NULL,"
               "captured_at INTEGER NOT NULL,"
               "resource_revision TEXT NOT NULL,"
               "payload_identity TEXT NOT NULL,"
               "canonical_payload TEXT NOT NULL,"
               "outcome TEXT NOT NULL,"
               "reason_code TEXT NOT NULL,"
               "accepted_at INTEGER NOT NULL,"
               "CHECK(kind IN ('completeSnapshot','changeBatch')),"
               "CHECK(outcome IN ('accepted','replayed','rejected','resync-required'))"
               ");") &&
        database_.execute(
               "CREATE UNIQUE INDEX IF NOT EXISTS idx_backend_agent_observation_accepted_key "
               "ON backend_agent_observation_receipts(backend_id, observation_domain, "
               "agent_id, agent_instance_id, backend_generation, snapshot_generation, "
               "producer_sequence) WHERE outcome = 'accepted';") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS idx_backend_agent_observation_receipt_lookup "
               "ON backend_agent_observation_receipts(backend_id, observation_domain, "
               "snapshot_generation, producer_sequence, outcome); ") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_observation_cursors ("
               "backend_id TEXT NOT NULL,"
               "observation_domain TEXT NOT NULL,"
               "agent_id TEXT NOT NULL,"
               "agent_instance_id TEXT NOT NULL,"
               "backend_generation INTEGER NOT NULL,"
               "snapshot_generation INTEGER NOT NULL,"
               "producer_sequence INTEGER NOT NULL,"
               "resource_revision TEXT NOT NULL,"
               "payload_identity TEXT NOT NULL,"
               "captured_at INTEGER NOT NULL,"
               "accepted_at INTEGER NOT NULL,"
               "PRIMARY KEY(backend_id, observation_domain)"
               ");") &&
        database_.execute(
               "CREATE TABLE IF NOT EXISTS backend_agent_channel_facts ("
               "backend_id TEXT NOT NULL,"
               "channel_id TEXT NOT NULL,"
               "channel_number INTEGER NOT NULL,"
               "name TEXT NOT NULL,"
               "provider TEXT NOT NULL,"
               "group_name TEXT NOT NULL,"
               "radio INTEGER NOT NULL,"
               "encrypted INTEGER NOT NULL,"
               "enabled INTEGER NOT NULL,"
               "agent_id TEXT NOT NULL,"
               "agent_instance_id TEXT NOT NULL,"
               "backend_generation INTEGER NOT NULL,"
               "snapshot_generation INTEGER NOT NULL,"
               "producer_sequence INTEGER NOT NULL,"
               "captured_at INTEGER NOT NULL,"
               "resource_revision TEXT NOT NULL,"
               "PRIMARY KEY(backend_id, channel_id),"
               "CHECK(radio IN (0,1)),"
               "CHECK(encrypted IN (0,1)),"
               "CHECK(enabled IN (0,1))"
               ");");
}

bool BackendAgentRepository::createEnrollment(
    const BackendAgentEnrollmentRecord& enrollment)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_agent_enrollments "
        "(enrollment_id, backend_id, token_hash, status, expires_at, created_at) "
        "VALUES (?, ?, ?, 'pending', ?, ?);";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindText(statement, 1, enrollment.enrollmentId) &&
        bindText(statement, 2, enrollment.backendId) &&
        bindText(statement, 3, enrollment.tokenHash) &&
        bindInt64(statement, 4, enrollment.expiresAt) &&
        bindInt64(statement, 5, enrollment.createdAt);
    if (!bound)
    {
        sqlite3_finalize(statement);
        return false;
    }
    return executeStatement(statement);
}

std::optional<BackendAgentEnrollmentRecord>
BackendAgentRepository::findEnrollment(const std::string& enrollmentId) const
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT enrollment_id, backend_id, token_hash, status, expires_at, agent_id "
        "FROM backend_agent_enrollments WHERE enrollment_id = ?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, enrollmentId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }

    std::optional<BackendAgentEnrollmentRecord> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        BackendAgentEnrollmentRecord enrollment;
        enrollment.enrollmentId = columnText(statement, 0);
        enrollment.backendId = columnText(statement, 1);
        enrollment.tokenHash = columnText(statement, 2);
        enrollment.status = columnText(statement, 3);
        enrollment.expiresAt = sqlite3_column_int64(statement, 4);
        enrollment.agentId = columnText(statement, 5);
        enrollment.createdAt = 0;
        result = enrollment;
    }
    sqlite3_finalize(statement);
    return result;
}

std::optional<BackendAgentRecord> BackendAgentRepository::findAgent(
    const std::string& agentId) const
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("SELECT ") + AgentColumns +
        " FROM backend_agents WHERE agent_id = ?;";
    if (sqlite3_prepare_v2(database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, agentId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }
    std::optional<BackendAgentRecord> result;
    if (sqlite3_step(statement) == SQLITE_ROW &&
        validAgentNumericColumns(statement))
    {
        result = readAgent(statement);
    }
    sqlite3_finalize(statement);
    return result;
}

std::optional<BackendAgentRecord> BackendAgentRepository::findAgentForBackend(
    const std::string& backendId) const
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql =
        std::string("SELECT ") + AgentColumns +
        " FROM backend_agents WHERE backend_id = ? "
        "ORDER BY (revoked_at = 0) DESC, updated_at DESC LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), sql.c_str(), -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }
    std::optional<BackendAgentRecord> result;
    if (sqlite3_step(statement) == SQLITE_ROW &&
        validAgentNumericColumns(statement))
    {
        result = readAgent(statement);
    }
    sqlite3_finalize(statement);
    return result;
}

BackendAgentCapabilityFacts BackendAgentRepository::capabilitiesForAgent(
    const std::string& agentId) const
{
    BackendAgentCapabilityFacts facts;
    facts.readOnly = true;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT capability_kind, capability_name, capability_value "
        "FROM backend_agent_capabilities WHERE agent_id = ? "
        "ORDER BY capability_kind, capability_name;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, agentId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return facts;
    }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string kind = columnText(statement, 0);
        const std::string name = columnText(statement, 1);
        const std::string value = columnText(statement, 2);
        if (kind == "adapter") facts.adapters.push_back(name);
        else if (kind == "observation") facts.observationDomains.push_back(name);
        else if (kind == "mode" && name == "read-only") facts.readOnly = value == "true";
    }
    sqlite3_finalize(statement);
    return facts;
}

bool BackendAgentRepository::bindEnrollmentInCurrentTransaction(
    const std::string& enrollmentId,
    const BackendAgentRecord& agent,
    std::int64_t consumedAt)
{
    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO backend_agents "
        "(agent_id, backend_id, actor_id, device_id, credential_id, "
        "credential_generation, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &insert, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool insertBound =
        bindText(insert, 1, agent.agentId) &&
        bindText(insert, 2, agent.backendId) &&
        bindText(insert, 3, agent.actorId) &&
        bindText(insert, 4, agent.deviceId) &&
        bindText(insert, 5, agent.credentialId) &&
        bindInt64(insert, 6, static_cast<std::int64_t>(agent.credentialGeneration)) &&
        bindInt64(insert, 7, consumedAt) &&
        bindInt64(insert, 8, consumedAt);
    if (!insertBound || !executeStatement(insert)) return false;

    sqlite3_stmt* update = nullptr;
    const char* updateSql =
        "UPDATE backend_agent_enrollments "
        "SET status = 'consumed', agent_id = ?, consumed_at = ? "
        "WHERE enrollment_id = ? AND status = 'pending';";
    if (sqlite3_prepare_v2(database_.handle(), updateSql, -1, &update, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool updateBound =
        bindText(update, 1, agent.agentId) &&
        bindInt64(update, 2, consumedAt) &&
        bindText(update, 3, enrollmentId);
    if (!updateBound || !executeStatement(update) || sqlite3_changes(database_.handle()) != 1)
    {
        return false;
    }
    return true;
}

bool BackendAgentRepository::revokeEnrollment(
    const std::string& enrollmentId,
    const std::string& reason,
    std::int64_t revokedAt)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE backend_agent_enrollments "
        "SET status = 'revoked', revoked_at = ?, revocation_reason = ? "
        "WHERE enrollment_id = ? AND status = 'pending';";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindInt64(statement, 1, revokedAt) &&
        bindText(statement, 2, reason) &&
        bindText(statement, 3, enrollmentId);
    return bound && executeStatement(statement) && sqlite3_changes(database_.handle()) == 1;
}

std::optional<std::uint64_t> BackendAgentRepository::credentialRotationGeneration(
    const std::string& agentId,
    const std::string& rotationId) const
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT to_generation FROM backend_agent_credential_rotations "
        "WHERE agent_id = ? AND rotation_id = ?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, agentId) || !bindText(statement, 2, rotationId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return std::nullopt;
    }
    std::optional<std::uint64_t> result;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        result = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 0));
    }
    sqlite3_finalize(statement);
    return result;
}

bool BackendAgentRepository::recordCredentialRotationInCurrentTransaction(
    const std::string& agentId,
    const std::string& rotationId,
    std::uint64_t fromGeneration,
    std::uint64_t toGeneration,
    std::int64_t rotatedAt)
{
    if (!fitsDatabaseInteger(fromGeneration) ||
        !fitsDatabaseInteger(toGeneration) || toGeneration != fromGeneration + 1)
    {
        return false;
    }
    sqlite3_stmt* insert = nullptr;
    const char* insertSql =
        "INSERT INTO backend_agent_credential_rotations "
        "(rotation_id, agent_id, from_generation, to_generation, rotated_at) "
        "VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &insert, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool insertBound =
        bindText(insert, 1, rotationId) &&
        bindText(insert, 2, agentId) &&
        bindInt64(insert, 3, static_cast<std::int64_t>(fromGeneration)) &&
        bindInt64(insert, 4, static_cast<std::int64_t>(toGeneration)) &&
        bindInt64(insert, 5, rotatedAt);
    if (!insertBound || !executeStatement(insert)) return false;

    sqlite3_stmt* update = nullptr;
    const char* updateSql =
        "UPDATE backend_agents SET credential_generation = ?, lease_expires_at = 0, "
        "updated_at = ? WHERE agent_id = ? AND credential_generation = ? "
        "AND revoked_at = 0;";
    if (sqlite3_prepare_v2(database_.handle(), updateSql, -1, &update, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool updateBound =
        bindInt64(update, 1, static_cast<std::int64_t>(toGeneration)) &&
        bindInt64(update, 2, rotatedAt) &&
        bindText(update, 3, agentId) &&
        bindInt64(update, 4, static_cast<std::int64_t>(fromGeneration));
    return updateBound && executeStatement(update) &&
        sqlite3_changes(database_.handle()) == 1;
}

bool BackendAgentRepository::markIncompatible(
    const std::string& agentId,
    const std::string& protocolVersion,
    const std::string& softwareVersion,
    std::int64_t connectedAt)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE backend_agents SET protocol_version = ?, software_version = ?, "
        "incompatible = 1, lease_expires_at = 0, updated_at = ? "
        "WHERE agent_id = ? AND revoked_at = 0;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindText(statement, 1, protocolVersion) &&
        bindText(statement, 2, softwareVersion) &&
        bindInt64(statement, 3, connectedAt) &&
        bindText(statement, 4, agentId);
    return bound && executeStatement(statement) && sqlite3_changes(database_.handle()) == 1;
}

bool BackendAgentRepository::acceptConnection(
    const std::string& agentId,
    const BackendAgentConnectRequest& request,
    std::int64_t connectedAt,
    BackendAgentConnectResult& result)
{
    Transaction transaction(database_);
    if (!transaction.active()) return false;
    const auto current = findAgent(agentId);
    if (!current.has_value() || current->revoked) return false;

    result.agentId = current->agentId;
    result.backendId = current->backendId;
    result.credentialGeneration = current->credentialGeneration;
    const bool sameInstance =
        !current->agentInstanceId.empty() &&
        current->agentInstanceId == request.agentInstanceId;

    if (sameInstance)
    {
        result.backendGeneration = current->backendGeneration;
        result.heartbeatSequence = current->heartbeatSequence;
        result.capabilityRevision = current->capabilityRevision;
        if ((request.claimedBackendGeneration != 0 &&
             request.claimedBackendGeneration != current->backendGeneration) ||
            request.claimedHeartbeatSequence > current->heartbeatSequence ||
            request.claimedCapabilityRevision > current->capabilityRevision)
        {
            result.disposition = "resync-required";
        }
        else
        {
            result.disposition = "resume";
        }
    }
    else
    {
        if (!fitsDatabaseInteger(current->backendGeneration) ||
            current->backendGeneration == static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()))
        {
            result.reasonCode = "backend_generation_exhausted";
            return transaction.commit();
        }
        result.backendGeneration = current->backendGeneration + 1;
        result.heartbeatSequence = 0;
        result.capabilityRevision = 0;
        result.disposition = "replace";
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql = sameInstance
        ? "UPDATE backend_agents SET protocol_version = ?, software_version = ?, "
          "last_connected_at = ?, incompatible = 0, updated_at = ? "
          "WHERE agent_id = ? AND revoked_at = 0;"
        : "UPDATE backend_agents SET agent_instance_id = ?, backend_generation = ?, "
          "protocol_version = ?, software_version = ?, heartbeat_sequence = 0, "
          "capability_revision = 0, last_connected_at = ?, last_heartbeat_at = 0, "
          "lease_expires_at = 0, incompatible = 0, updated_at = ? "
          "WHERE agent_id = ? AND revoked_at = 0;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    bool bound = false;
    if (sameInstance)
    {
        bound =
            bindText(statement, 1, request.protocolVersion) &&
            bindText(statement, 2, request.softwareVersion) &&
            bindInt64(statement, 3, connectedAt) &&
            bindInt64(statement, 4, connectedAt) &&
            bindText(statement, 5, agentId);
    }
    else
    {
        bound =
            bindText(statement, 1, request.agentInstanceId) &&
            bindInt64(statement, 2, static_cast<std::int64_t>(result.backendGeneration)) &&
            bindText(statement, 3, request.protocolVersion) &&
            bindText(statement, 4, request.softwareVersion) &&
            bindInt64(statement, 5, connectedAt) &&
            bindInt64(statement, 6, connectedAt) &&
            bindText(statement, 7, agentId);
    }
    if (!bound || !executeStatement(statement) || sqlite3_changes(database_.handle()) != 1)
    {
        return false;
    }
    if (!sameInstance)
    {
        sqlite3_stmt* erase = nullptr;
        if (sqlite3_prepare_v2(
                database_.handle(),
                "DELETE FROM backend_agent_capabilities WHERE agent_id = ?;",
                -1,
                &erase,
                nullptr) != SQLITE_OK ||
            !bindText(erase, 1, agentId) ||
            !executeStatement(erase))
        {
            if (erase != nullptr) sqlite3_finalize(erase);
            return false;
        }
    }
    result.accepted = true;
    return transaction.commit();
}

bool BackendAgentRepository::renewLease(
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::uint64_t heartbeatSequence,
    std::int64_t acceptedAt,
    std::int64_t leaseExpiresAt,
    BackendAgentHeartbeatResult& result)
{
    if (!fitsDatabaseInteger(backendGeneration) ||
        !fitsDatabaseInteger(heartbeatSequence))
    {
        result.reasonCode = "agent_sequence_out_of_range";
        return true;
    }
    Transaction transaction(database_);
    if (!transaction.active()) return false;
    const auto current = findAgent(agentId);
    if (!current.has_value() || current->revoked || current->incompatible ||
        current->agentInstanceId != agentInstanceId ||
        current->backendGeneration != backendGeneration)
    {
        result.reasonCode = "stale_agent_instance";
        return transaction.commit();
    }
    if (heartbeatSequence == current->heartbeatSequence)
    {
        result.accepted = true;
        result.duplicate = true;
        result.reasonCode = "duplicate_heartbeat";
        result.heartbeatSequence = current->heartbeatSequence;
        result.leaseExpiresAt = current->leaseExpiresAt;
        return transaction.commit();
    }
    if (heartbeatSequence < current->heartbeatSequence)
    {
        result.reasonCode = "stale_heartbeat_sequence";
        return transaction.commit();
    }
    if (heartbeatSequence != current->heartbeatSequence + 1)
    {
        result.reasonCode = "heartbeat_sequence_gap";
        return transaction.commit();
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE backend_agents SET heartbeat_sequence = ?, last_heartbeat_at = ?, "
        "lease_expires_at = ?, updated_at = ? "
        "WHERE agent_id = ? AND agent_instance_id = ? AND backend_generation = ? "
        "AND revoked_at = 0 AND incompatible = 0;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindInt64(statement, 1, static_cast<std::int64_t>(heartbeatSequence)) &&
        bindInt64(statement, 2, acceptedAt) &&
        bindInt64(statement, 3, leaseExpiresAt) &&
        bindInt64(statement, 4, acceptedAt) &&
        bindText(statement, 5, agentId) &&
        bindText(statement, 6, agentInstanceId) &&
        bindInt64(statement, 7, static_cast<std::int64_t>(backendGeneration));
    if (!bound || !executeStatement(statement) || sqlite3_changes(database_.handle()) != 1)
    {
        return false;
    }
    result.accepted = true;
    result.reasonCode = "lease_renewed";
    result.heartbeatSequence = heartbeatSequence;
    result.leaseExpiresAt = leaseExpiresAt;
    return transaction.commit();
}

bool BackendAgentRepository::publishCapabilities(
    const std::string& agentId,
    const std::string& agentInstanceId,
    std::uint64_t backendGeneration,
    std::uint64_t capabilityRevision,
    const BackendAgentCapabilityFacts& facts,
    std::int64_t publishedAt,
    BackendAgentCapabilityResult& result)
{
    if (!fitsDatabaseInteger(backendGeneration) ||
        !fitsDatabaseInteger(capabilityRevision))
    {
        result.reasonCode = "agent_sequence_out_of_range";
        return true;
    }
    Transaction transaction(database_);
    if (!transaction.active()) return false;
    const auto current = findAgent(agentId);
    if (!current.has_value() || current->revoked || current->incompatible ||
        current->agentInstanceId != agentInstanceId ||
        current->backendGeneration != backendGeneration)
    {
        result.reasonCode = "stale_agent_instance";
        return transaction.commit();
    }

    if (capabilityRevision == current->capabilityRevision)
    {
        const BackendAgentCapabilityFacts existing = capabilitiesForAgent(agentId);
        std::set<std::string> existingAdapters(existing.adapters.begin(), existing.adapters.end());
        std::set<std::string> wantedAdapters(facts.adapters.begin(), facts.adapters.end());
        std::set<std::string> existingDomains(
            existing.observationDomains.begin(), existing.observationDomains.end());
        std::set<std::string> wantedDomains(
            facts.observationDomains.begin(), facts.observationDomains.end());
        if (existing.readOnly == facts.readOnly &&
            existingAdapters == wantedAdapters &&
            existingDomains == wantedDomains)
        {
            result.accepted = true;
            result.duplicate = true;
            result.reasonCode = "duplicate_capabilities";
            result.capabilityRevision = current->capabilityRevision;
        }
        else
        {
            result.reasonCode = "capability_revision_conflict";
        }
        return transaction.commit();
    }
    if (capabilityRevision < current->capabilityRevision)
    {
        result.reasonCode = "stale_capability_revision";
        return transaction.commit();
    }
    if (capabilityRevision != current->capabilityRevision + 1)
    {
        result.reasonCode = "capability_revision_gap";
        return transaction.commit();
    }

    sqlite3_stmt* erase = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            "DELETE FROM backend_agent_capabilities WHERE agent_id = ?;",
            -1,
            &erase,
            nullptr) != SQLITE_OK ||
        !bindText(erase, 1, agentId) ||
        !executeStatement(erase))
    {
        if (erase != nullptr) sqlite3_finalize(erase);
        return false;
    }

    const char* insertSql =
        "INSERT INTO backend_agent_capabilities "
        "(agent_id, capability_revision, capability_kind, capability_name, capability_value) "
        "VALUES (?, ?, ?, ?, ?);";
    auto insertFact = [&](const std::string& kind, const std::string& name, const std::string& value)
    {
        sqlite3_stmt* statement = nullptr;
        if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &statement, nullptr) != SQLITE_OK)
        {
            return false;
        }
        const bool bound =
            bindText(statement, 1, agentId) &&
            bindInt64(statement, 2, static_cast<std::int64_t>(capabilityRevision)) &&
            bindText(statement, 3, kind) &&
            bindText(statement, 4, name) &&
            bindText(statement, 5, value);
        return bound && executeStatement(statement);
    };

    if (!insertFact("mode", "read-only", facts.readOnly ? "true" : "false")) return false;
    for (const std::string& adapter : facts.adapters)
    {
        if (!insertFact("adapter", adapter, "true")) return false;
    }
    for (const std::string& domain : facts.observationDomains)
    {
        if (!insertFact("observation", domain, "true")) return false;
    }

    sqlite3_stmt* update = nullptr;
    const char* updateSql =
        "UPDATE backend_agents SET capability_revision = ?, "
        "updated_at = ? "
        "WHERE agent_id = ? AND agent_instance_id = ? AND backend_generation = ?;";
    if (sqlite3_prepare_v2(database_.handle(), updateSql, -1, &update, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool updateBound =
        bindInt64(update, 1, static_cast<std::int64_t>(capabilityRevision)) &&
        bindInt64(update, 2, publishedAt) &&
        bindText(update, 3, agentId) &&
        bindText(update, 4, agentInstanceId) &&
        bindInt64(update, 5, static_cast<std::int64_t>(backendGeneration));
    if (!updateBound || !executeStatement(update) || sqlite3_changes(database_.handle()) != 1)
    {
        return false;
    }

    result.accepted = true;
    result.reasonCode = "capabilities_published";
    result.capabilityRevision = capabilityRevision;
    return transaction.commit();
}

bool BackendAgentRepository::revokeAgent(
    const std::string& agentId,
    const std::string& reason,
    std::int64_t revokedAt)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE backend_agents SET revoked_at = ?, revocation_reason = ?, "
        "lease_expires_at = 0, updated_at = ? WHERE agent_id = ? AND revoked_at = 0;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }
    const bool bound =
        bindInt64(statement, 1, revokedAt) &&
        bindText(statement, 2, reason) &&
        bindInt64(statement, 3, revokedAt) &&
        bindText(statement, 4, agentId);
    return bound && executeStatement(statement) && sqlite3_changes(database_.handle()) == 1;
}


BackendAgentObservationCursor BackendAgentRepository::observationCursorForBackend(
    const std::string& backendId,
    const std::string& observationDomain) const
{
    BackendAgentObservationCursor cursor;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT backend_id, observation_domain, agent_id, agent_instance_id, "
        "backend_generation, snapshot_generation, producer_sequence, resource_revision, "
        "payload_identity, captured_at, accepted_at "
        "FROM backend_agent_observation_cursors "
        "WHERE backend_id = ? AND observation_domain = ?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId) ||
        !bindText(statement, 2, observationDomain))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return cursor;
    }
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::int64_t backendGeneration = sqlite3_column_int64(statement, 4);
        const std::int64_t snapshotGeneration = sqlite3_column_int64(statement, 5);
        const std::int64_t producerSequence = sqlite3_column_int64(statement, 6);
        if (backendGeneration >= 0 && snapshotGeneration >= 0 && producerSequence >= 0)
        {
            cursor.present = true;
            cursor.backendId = columnText(statement, 0);
            cursor.observationDomain = columnText(statement, 1);
            cursor.agentId = columnText(statement, 2);
            cursor.agentInstanceId = columnText(statement, 3);
            cursor.backendGeneration = static_cast<std::uint64_t>(backendGeneration);
            cursor.snapshotGeneration = static_cast<std::uint64_t>(snapshotGeneration);
            cursor.producerSequence = static_cast<std::uint64_t>(producerSequence);
            cursor.resourceRevision = columnText(statement, 7);
            cursor.payloadIdentity = columnText(statement, 8);
            cursor.capturedAt = sqlite3_column_int64(statement, 9);
            cursor.acceptedAt = sqlite3_column_int64(statement, 10);
        }
    }
    sqlite3_finalize(statement);
    return cursor;
}

std::vector<BackendAgentChannelFact> BackendAgentRepository::channelFactsForBackend(
    const std::string& backendId) const
{
    std::vector<BackendAgentChannelFact> facts;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT channel_id, channel_number, name, provider, group_name, "
        "radio, encrypted, enabled FROM backend_agent_channel_facts "
        "WHERE backend_id = ? ORDER BY channel_id;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, backendId))
    {
        if (statement != nullptr) sqlite3_finalize(statement);
        return facts;
    }
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::int64_t channelNumber = sqlite3_column_int64(statement, 1);
        if (channelNumber <= 0)
        {
            facts.clear();
            break;
        }
        BackendAgentChannelFact fact;
        fact.channelId = columnText(statement, 0);
        fact.channelNumber = static_cast<std::uint64_t>(channelNumber);
        fact.name = columnText(statement, 2);
        fact.provider = columnText(statement, 3);
        fact.groupName = columnText(statement, 4);
        fact.radio = sqlite3_column_int(statement, 5) != 0;
        fact.encrypted = sqlite3_column_int(statement, 6) != 0;
        fact.enabled = sqlite3_column_int(statement, 7) != 0;
        facts.push_back(std::move(fact));
    }
    sqlite3_finalize(statement);
    return facts;
}

bool BackendAgentRepository::ingestObservation(
    const std::string& agentId,
    const BackendAgentObservationRequest& request,
    const std::string& payloadIdentity,
    const std::string& canonicalPayload,
    std::int64_t acceptedAt,
    BackendAgentObservationResult& result)
{
    if (!fitsDatabaseInteger(request.backendGeneration) ||
        !fitsDatabaseInteger(request.snapshotGeneration) ||
        !fitsDatabaseInteger(request.producerSequence) ||
        !fitsDatabaseInteger(request.observedHeartbeatSequence))
    {
        result.reasonCode = "observation_sequence_out_of_range";
        return true;
    }
    Transaction transaction(database_);
    if (!transaction.active()) return false;

    const auto agent = findAgent(agentId);
    auto classify = [&](const std::string& reason, bool resync = false) {
        result.accepted = false;
        result.replayed = false;
        result.resyncRequired = resync;
        result.reasonCode = reason;
    };
    if (!agent.has_value() || agent->revoked || agent->incompatible)
        classify("agent_revoked_or_unknown");
    else if (agent->backendId != request.backendId)
        classify("agent_binding_mismatch");
    else if (agent->agentInstanceId != request.agentInstanceId)
        classify("stale_agent_instance");
    else if (agent->backendGeneration != request.backendGeneration)
        classify("stale_backend_generation");
    else if (agent->leaseExpiresAt <= acceptedAt)
        classify("agent_lease_expired");
    else if (request.observedHeartbeatSequence != agent->heartbeatSequence)
        classify("stale_backend_health_revision");
    else
    {
        const BackendAgentCapabilityFacts capabilities = capabilitiesForAgent(agentId);
        const bool declared = std::find(
            capabilities.observationDomains.begin(),
            capabilities.observationDomains.end(),
            request.observationDomain) != capabilities.observationDomains.end();
        if (!capabilities.readOnly || !declared)
            classify("undeclared_observation_domain");
    }

    BackendAgentObservationCursor cursor;
    if (result.reasonCode.empty())
        cursor = observationCursorForBackend(request.backendId, request.observationDomain);

    std::string existingKind;
    std::int64_t existingCapturedAt = 0;
    std::string existingResourceRevision;
    std::string existingIdentity;
    std::string existingPayload;
    bool existingAccepted = false;
    if (result.reasonCode.empty())
    {
        sqlite3_stmt* lookup = nullptr;
        const char* lookupSql =
            "SELECT kind, captured_at, resource_revision, payload_identity, canonical_payload "
            "FROM backend_agent_observation_receipts "
            "WHERE backend_id = ? AND observation_domain = ? AND agent_id = ? "
            "AND agent_instance_id = ? AND backend_generation = ? "
            "AND snapshot_generation = ? AND producer_sequence = ? "
            "AND outcome = 'accepted' LIMIT 1;";
        if (sqlite3_prepare_v2(database_.handle(), lookupSql, -1, &lookup, nullptr) != SQLITE_OK ||
            !bindText(lookup, 1, request.backendId) ||
            !bindText(lookup, 2, request.observationDomain) ||
            !bindText(lookup, 3, agentId) ||
            !bindText(lookup, 4, request.agentInstanceId) ||
            !bindInt64(lookup, 5, static_cast<std::int64_t>(request.backendGeneration)) ||
            !bindInt64(lookup, 6, static_cast<std::int64_t>(request.snapshotGeneration)) ||
            !bindInt64(lookup, 7, static_cast<std::int64_t>(request.producerSequence)))
        {
            if (lookup != nullptr) sqlite3_finalize(lookup);
            return false;
        }
        if (sqlite3_step(lookup) == SQLITE_ROW)
        {
            existingAccepted = true;
            existingKind = columnText(lookup, 0);
            existingCapturedAt = sqlite3_column_int64(lookup, 1);
            existingResourceRevision = columnText(lookup, 2);
            existingIdentity = columnText(lookup, 3);
            existingPayload = columnText(lookup, 4);
        }
        sqlite3_finalize(lookup);
    }

    bool advanceCursor = false;
    if (result.reasonCode.empty() && existingAccepted)
    {
        if (existingKind == request.kind &&
            existingCapturedAt == request.capturedAt &&
            existingResourceRevision == request.resourceRevision &&
            existingIdentity == payloadIdentity && existingPayload == canonicalPayload)
        {
            result.accepted = true;
            result.replayed = true;
            result.reasonCode = "observation_replayed";
        }
        else classify("observation_replay_conflict");
    }
    else if (result.reasonCode.empty() && request.kind == "completeSnapshot")
    {
        if (request.producerSequence != 1)
            classify("invalid_snapshot_sequence");
        else if (cursor.present && cursor.agentId == agentId &&
                 cursor.agentInstanceId == request.agentInstanceId &&
                 cursor.backendGeneration == request.backendGeneration &&
                 request.snapshotGeneration <= cursor.snapshotGeneration)
            classify("stale_snapshot_generation");
        else
        {
            result.accepted = true;
            result.reasonCode = "complete_snapshot_accepted";
            advanceCursor = true;
        }
    }
    else if (result.reasonCode.empty() && request.kind == "changeBatch")
    {
        if (!cursor.present || cursor.agentId != agentId ||
            cursor.agentInstanceId != request.agentInstanceId ||
            cursor.backendGeneration != request.backendGeneration)
            classify("observation_baseline_required", true);
        else if (request.snapshotGeneration < cursor.snapshotGeneration)
            classify("stale_snapshot_generation");
        else if (request.snapshotGeneration > cursor.snapshotGeneration)
            classify("observation_resync_required", true);
        else if (request.producerSequence <= cursor.producerSequence)
            classify("stale_observation_sequence");
        else if (cursor.producerSequence == static_cast<std::uint64_t>(
                     std::numeric_limits<std::int64_t>::max()) ||
                 request.producerSequence != cursor.producerSequence + 1)
            classify("observation_resync_required", true);
        else
        {
            result.accepted = true;
            result.reasonCode = "change_batch_accepted";
            advanceCursor = true;
        }
    }
    else if (result.reasonCode.empty())
        classify("invalid_observation_kind");

    if (result.accepted && advanceCursor &&
        request.observationDomain == "channels" && request.kind == "changeBatch")
    {
        for (const std::string& channelId : request.removedChannelIds)
        {
            sqlite3_stmt* lookupChannel = nullptr;
            const char* lookupChannelSql =
                "SELECT 1 FROM backend_agent_channel_facts "
                "WHERE backend_id = ? AND channel_id = ? LIMIT 1;";
            if (sqlite3_prepare_v2(
                    database_.handle(), lookupChannelSql, -1, &lookupChannel, nullptr) !=
                    SQLITE_OK ||
                !bindText(lookupChannel, 1, request.backendId) ||
                !bindText(lookupChannel, 2, channelId))
            {
                if (lookupChannel != nullptr) sqlite3_finalize(lookupChannel);
                return false;
            }
            const bool present = sqlite3_step(lookupChannel) == SQLITE_ROW;
            sqlite3_finalize(lookupChannel);
            if (!present)
            {
                classify("unknown_channel_removal");
                advanceCursor = false;
                break;
            }
        }
    }

    result.snapshotGeneration = request.snapshotGeneration;
    result.producerSequence = request.producerSequence;
    result.lastAcceptedSequence = cursor.present ? cursor.producerSequence : 0;

    const std::string outcome = result.replayed ? "replayed" :
        result.accepted ? "accepted" :
        result.resyncRequired ? "resync-required" : "rejected";
    sqlite3_stmt* receipt = nullptr;
    const char* receiptSql =
        "INSERT INTO backend_agent_observation_receipts "
        "(backend_id, observation_domain, agent_id, agent_instance_id, backend_generation, "
        "snapshot_generation, producer_sequence, kind, captured_at, resource_revision, "
        "payload_identity, canonical_payload, outcome, reason_code, accepted_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(database_.handle(), receiptSql, -1, &receipt, nullptr) != SQLITE_OK)
        return false;
    const bool receiptBound =
        bindText(receipt, 1, request.backendId) &&
        bindText(receipt, 2, request.observationDomain) &&
        bindText(receipt, 3, agentId) &&
        bindText(receipt, 4, request.agentInstanceId) &&
        bindInt64(receipt, 5, static_cast<std::int64_t>(request.backendGeneration)) &&
        bindInt64(receipt, 6, static_cast<std::int64_t>(request.snapshotGeneration)) &&
        bindInt64(receipt, 7, static_cast<std::int64_t>(request.producerSequence)) &&
        bindText(receipt, 8, request.kind) &&
        bindInt64(receipt, 9, request.capturedAt) &&
        bindText(receipt, 10, request.resourceRevision) &&
        bindText(receipt, 11, payloadIdentity) &&
        bindText(receipt, 12, canonicalPayload) &&
        bindText(receipt, 13, outcome) &&
        bindText(receipt, 14, result.reasonCode) &&
        bindInt64(receipt, 15, acceptedAt);
    if (!receiptBound || !executeStatement(receipt)) return false;

    if (advanceCursor && request.observationDomain == "channels")
    {
        auto insertChannel = [&](const BackendAgentChannelFact& fact) {
            sqlite3_stmt* insert = nullptr;
            const char* insertSql =
                "INSERT INTO backend_agent_channel_facts "
                "(backend_id, channel_id, channel_number, name, provider, group_name, "
                "radio, encrypted, enabled, agent_id, agent_instance_id, "
                "backend_generation, snapshot_generation, producer_sequence, captured_at, "
                "resource_revision) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
                "ON CONFLICT(backend_id, channel_id) DO UPDATE SET "
                "channel_number=excluded.channel_number, name=excluded.name, "
                "provider=excluded.provider, group_name=excluded.group_name, "
                "radio=excluded.radio, encrypted=excluded.encrypted, enabled=excluded.enabled, "
                "agent_id=excluded.agent_id, agent_instance_id=excluded.agent_instance_id, "
                "backend_generation=excluded.backend_generation, "
                "snapshot_generation=excluded.snapshot_generation, "
                "producer_sequence=excluded.producer_sequence, "
                "captured_at=excluded.captured_at, "
                "resource_revision=excluded.resource_revision;";
            if (sqlite3_prepare_v2(database_.handle(), insertSql, -1, &insert, nullptr) !=
                SQLITE_OK)
            {
                return false;
            }
            const bool bound =
                bindText(insert, 1, request.backendId) &&
                bindText(insert, 2, fact.channelId) &&
                bindInt64(insert, 3, static_cast<std::int64_t>(fact.channelNumber)) &&
                bindText(insert, 4, fact.name) &&
                bindText(insert, 5, fact.provider) &&
                bindText(insert, 6, fact.groupName) &&
                sqlite3_bind_int(insert, 7, fact.radio ? 1 : 0) == SQLITE_OK &&
                sqlite3_bind_int(insert, 8, fact.encrypted ? 1 : 0) == SQLITE_OK &&
                sqlite3_bind_int(insert, 9, fact.enabled ? 1 : 0) == SQLITE_OK &&
                bindText(insert, 10, agentId) &&
                bindText(insert, 11, request.agentInstanceId) &&
                bindInt64(insert, 12, static_cast<std::int64_t>(request.backendGeneration)) &&
                bindInt64(insert, 13, static_cast<std::int64_t>(request.snapshotGeneration)) &&
                bindInt64(insert, 14, static_cast<std::int64_t>(request.producerSequence)) &&
                bindInt64(insert, 15, request.capturedAt) &&
                bindText(insert, 16, request.resourceRevision);
            return bound && executeStatement(insert);
        };

        if (request.kind == "completeSnapshot")
        {
            sqlite3_stmt* clear = nullptr;
            if (sqlite3_prepare_v2(
                    database_.handle(),
                    "DELETE FROM backend_agent_channel_facts WHERE backend_id = ?;",
                    -1, &clear, nullptr) != SQLITE_OK)
            {
                return false;
            }
            if (!bindText(clear, 1, request.backendId))
            {
                sqlite3_finalize(clear);
                return false;
            }
            if (!executeStatement(clear)) return false;
            for (const auto& fact : request.channels)
            {
                if (!insertChannel(fact)) return false;
            }
        }
        else
        {
            for (const std::string& channelId : request.removedChannelIds)
            {
                sqlite3_stmt* remove = nullptr;
                if (sqlite3_prepare_v2(
                        database_.handle(),
                        "DELETE FROM backend_agent_channel_facts "
                        "WHERE backend_id = ? AND channel_id = ?;",
                        -1, &remove, nullptr) != SQLITE_OK)
                {
                    return false;
                }
                if (!bindText(remove, 1, request.backendId) ||
                    !bindText(remove, 2, channelId))
                {
                    sqlite3_finalize(remove);
                    return false;
                }
                if (!executeStatement(remove) || sqlite3_changes(database_.handle()) != 1)
                    return false;
            }
            for (const auto& fact : request.upserts)
            {
                if (!insertChannel(fact)) return false;
            }
        }
    }

    if (advanceCursor)
    {
        sqlite3_stmt* update = nullptr;
        const char* updateSql =
            "INSERT INTO backend_agent_observation_cursors "
            "(backend_id, observation_domain, agent_id, agent_instance_id, backend_generation, "
            "snapshot_generation, producer_sequence, resource_revision, payload_identity, "
            "captured_at, accepted_at) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
            "ON CONFLICT(backend_id, observation_domain) DO UPDATE SET "
            "agent_id=excluded.agent_id, agent_instance_id=excluded.agent_instance_id, "
            "backend_generation=excluded.backend_generation, "
            "snapshot_generation=excluded.snapshot_generation, "
            "producer_sequence=excluded.producer_sequence, "
            "resource_revision=excluded.resource_revision, "
            "payload_identity=excluded.payload_identity, captured_at=excluded.captured_at, "
            "accepted_at=excluded.accepted_at;";
        if (sqlite3_prepare_v2(database_.handle(), updateSql, -1, &update, nullptr) != SQLITE_OK)
            return false;
        const bool updateBound =
            bindText(update, 1, request.backendId) &&
            bindText(update, 2, request.observationDomain) &&
            bindText(update, 3, agentId) &&
            bindText(update, 4, request.agentInstanceId) &&
            bindInt64(update, 5, static_cast<std::int64_t>(request.backendGeneration)) &&
            bindInt64(update, 6, static_cast<std::int64_t>(request.snapshotGeneration)) &&
            bindInt64(update, 7, static_cast<std::int64_t>(request.producerSequence)) &&
            bindText(update, 8, request.resourceRevision) &&
            bindText(update, 9, payloadIdentity) &&
            bindInt64(update, 10, request.capturedAt) &&
            bindInt64(update, 11, acceptedAt);
        if (!updateBound || !executeStatement(update)) return false;
        result.lastAcceptedSequence = request.producerSequence;
    }
    return transaction.commit();
}
