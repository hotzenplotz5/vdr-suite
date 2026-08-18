#include "MediaSessionRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

bool safePart(const std::string& value, std::size_t maximumLength)
{
    if (value.empty() || value.size() > maximumLength) {
        return false;
    }
    return std::none_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return character == '\0' || character == '\r' ||
                character == '\n' || std::iscntrl(character);
        });
}

bool safeIdentifier(const std::string& value)
{
    if (value.empty() || value.size() > 128) {
        return false;
    }
    return std::all_of(
        value.begin(), value.end(),
        [](unsigned char character) {
            return std::isalnum(character) || character == '-' ||
                character == '_' || character == '.' || character == ':';
        });
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string{}
        : std::string(reinterpret_cast<const char*>(value));
}

class DatabaseTransaction
{
public:
    explicit DatabaseTransaction(Database& database)
        : database_(database), active_(database_.execute("BEGIN IMMEDIATE;"))
    {
    }

    ~DatabaseTransaction()
    {
        if (active_) {
            database_.execute("ROLLBACK;");
        }
    }

    bool active() const { return active_; }

    bool commit()
    {
        if (!active_ || !database_.execute("COMMIT;")) {
            return false;
        }
        active_ = false;
        return true;
    }

private:
    Database& database_;
    bool active_ = false;
};

bool executeBound(
    Database& database,
    const char* sql,
    const std::vector<std::string>& values)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }

    bool bound = true;
    for (std::size_t index = 0; index < values.size(); ++index) {
        bound = bound && bindText(statement, static_cast<int>(index + 1), values[index]);
    }

    const int result = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    sqlite3_finalize(statement);
    return result == SQLITE_DONE;
}

bool validRegistration(const MediaSessionBundleRegistration& value)
{
    return safeIdentifier(value.sessionId) &&
        safeIdentifier(value.actorId) &&
        safeIdentifier(value.backendId) &&
        safeIdentifier(value.resourceKind) &&
        safePart(value.resourceId, 512) &&
        safeIdentifier(value.presentationProfileId) &&
        safeIdentifier(value.routeId) &&
        value.routeEpoch > 0 &&
        safeIdentifier(value.providerId) &&
        safeIdentifier(value.leaseId) &&
        safeIdentifier(value.workspaceId) &&
        safeIdentifier(value.grantId) &&
        MediaSessionRepository::supportsSecretHash(value.secretHash) &&
        safePart(value.expiresAt, 64);
}

} // namespace

MediaSessionRepository::MediaSessionRepository(Database& database)
    : database_(database)
{
}

bool MediaSessionRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS media_sessions ("
        "session_id TEXT PRIMARY KEY,"
        "actor_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "resource_kind TEXT NOT NULL,"
        "resource_id TEXT NOT NULL,"
        "state TEXT NOT NULL,"
        "presentation_profile_id TEXT NOT NULL,"
        "terminal_reason TEXT NOT NULL DEFAULT '',"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "ended_at TEXT NOT NULL DEFAULT ''"
        ");") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS media_routes ("
        "route_id TEXT PRIMARY KEY,"
        "session_id TEXT NOT NULL UNIQUE,"
        "route_epoch INTEGER NOT NULL,"
        "provider_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "state TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(session_id) REFERENCES media_sessions(session_id)"
        ");") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS media_provider_stream_leases ("
        "lease_id TEXT PRIMARY KEY,"
        "route_id TEXT NOT NULL UNIQUE,"
        "provider_id TEXT NOT NULL,"
        "workspace_id TEXT NOT NULL,"
        "state TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "ended_at TEXT NOT NULL DEFAULT '',"
        "FOREIGN KEY(route_id) REFERENCES media_routes(route_id)"
        ");") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS media_access_grants ("
        "grant_id TEXT PRIMARY KEY,"
        "session_id TEXT NOT NULL UNIQUE,"
        "route_id TEXT NOT NULL,"
        "route_epoch INTEGER NOT NULL,"
        "actor_id TEXT NOT NULL,"
        "secret_hash TEXT NOT NULL,"
        "expires_at TEXT NOT NULL,"
        "last_seen_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "active INTEGER NOT NULL DEFAULT 0,"
        "revoked_at TEXT NOT NULL DEFAULT '',"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "FOREIGN KEY(session_id) REFERENCES media_sessions(session_id),"
        "FOREIGN KEY(route_id) REFERENCES media_routes(route_id)"
        ");") &&
        database_.execute(
        "CREATE INDEX IF NOT EXISTS idx_media_sessions_actor_state "
        "ON media_sessions(actor_id, state, updated_at);") &&
        database_.execute(
        "CREATE INDEX IF NOT EXISTS idx_media_access_grants_active "
        "ON media_access_grants(grant_id, active, expires_at, last_seen_at);");
}

bool MediaSessionRepository::insertProvisioningBundle(
    const MediaSessionBundleRegistration& value)
{
    if (!validRegistration(value)) {
        return false;
    }

    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) {
        return false;
    }

    sqlite3_stmt* session = nullptr;
    const char* sessionSql =
        "INSERT INTO media_sessions "
        "(session_id, actor_id, backend_id, resource_kind, resource_id, state, presentation_profile_id) "
        "VALUES (?, ?, ?, ?, ?, 'provisioning', ?);";
    if (sqlite3_prepare_v2(database_.handle(), sessionSql, -1, &session, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool sessionBound =
        bindText(session, 1, value.sessionId) && bindText(session, 2, value.actorId) &&
        bindText(session, 3, value.backendId) && bindText(session, 4, value.resourceKind) &&
        bindText(session, 5, value.resourceId) && bindText(session, 6, value.presentationProfileId);
    const int sessionResult = sessionBound ? sqlite3_step(session) : SQLITE_ERROR;
    sqlite3_finalize(session);
    if (sessionResult != SQLITE_DONE) {
        return false;
    }

    sqlite3_stmt* route = nullptr;
    const char* routeSql =
        "INSERT INTO media_routes "
        "(route_id, session_id, route_epoch, provider_id, backend_id, state) "
        "VALUES (?, ?, ?, ?, ?, 'provisioning');";
    if (sqlite3_prepare_v2(database_.handle(), routeSql, -1, &route, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool routeBound =
        bindText(route, 1, value.routeId) && bindText(route, 2, value.sessionId) &&
        sqlite3_bind_int64(route, 3, value.routeEpoch) == SQLITE_OK &&
        bindText(route, 4, value.providerId) && bindText(route, 5, value.backendId);
    const int routeResult = routeBound ? sqlite3_step(route) : SQLITE_ERROR;
    sqlite3_finalize(route);
    if (routeResult != SQLITE_DONE) {
        return false;
    }

    if (!executeBound(
            database_,
            "INSERT INTO media_provider_stream_leases "
            "(lease_id, route_id, provider_id, workspace_id, state) "
            "VALUES (?, ?, ?, ?, 'provisioning');",
            {value.leaseId, value.routeId, value.providerId, value.workspaceId})) {
        return false;
    }

    sqlite3_stmt* grant = nullptr;
    const char* grantSql =
        "INSERT INTO media_access_grants "
        "(grant_id, session_id, route_id, route_epoch, actor_id, secret_hash, expires_at, active) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, 0);";
    if (sqlite3_prepare_v2(database_.handle(), grantSql, -1, &grant, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool grantBound =
        bindText(grant, 1, value.grantId) && bindText(grant, 2, value.sessionId) &&
        bindText(grant, 3, value.routeId) &&
        sqlite3_bind_int64(grant, 4, value.routeEpoch) == SQLITE_OK &&
        bindText(grant, 5, value.actorId) && bindText(grant, 6, value.secretHash) &&
        bindText(grant, 7, value.expiresAt);
    const int grantResult = grantBound ? sqlite3_step(grant) : SQLITE_ERROR;
    sqlite3_finalize(grant);
    if (grantResult != SQLITE_DONE) {
        return false;
    }

    return transaction.commit();
}

bool MediaSessionRepository::activateBundle(const std::string& sessionId)
{
    if (!safeIdentifier(sessionId)) {
        return false;
    }
    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) {
        return false;
    }

    const bool session = executeBound(
        database_,
        "UPDATE media_sessions SET state='ready', updated_at=CURRENT_TIMESTAMP "
        "WHERE session_id=? AND state='provisioning';",
        {sessionId});
    if (!session || sqlite3_changes(database_.handle()) != 1) {
        return false;
    }
    if (!executeBound(
            database_,
            "UPDATE media_routes SET state='ready', updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND state='provisioning';",
            {sessionId}) || sqlite3_changes(database_.handle()) != 1) {
        return false;
    }
    if (!executeBound(
            database_,
            "UPDATE media_provider_stream_leases SET state='active', updated_at=CURRENT_TIMESTAMP "
            "WHERE route_id=(SELECT route_id FROM media_routes WHERE session_id=?) "
            "AND state='provisioning';",
            {sessionId}) || sqlite3_changes(database_.handle()) != 1) {
        return false;
    }
    if (!executeBound(
            database_,
            "UPDATE media_access_grants SET active=1, revoked_at='', updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND active=0 AND revoked_at='' AND expires_at>CURRENT_TIMESTAMP;",
            {sessionId}) || sqlite3_changes(database_.handle()) != 1) {
        return false;
    }
    return transaction.commit();
}

bool MediaSessionRepository::failBundle(
    const std::string& sessionId,
    const std::string& reasonCode)
{
    if (!safeIdentifier(sessionId) || !safeIdentifier(reasonCode)) {
        return false;
    }
    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) {
        return false;
    }

    if (!executeBound(
            database_,
            "UPDATE media_access_grants SET active=0, revoked_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND revoked_at='';",
            {sessionId}) ||
        !executeBound(
            database_,
            "UPDATE media_provider_stream_leases SET state='failed', ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
            "WHERE route_id=(SELECT route_id FROM media_routes WHERE session_id=?) AND state NOT IN ('ended','failed');",
            {sessionId}) ||
        !executeBound(
            database_,
            "UPDATE media_routes SET state='failed', updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND state NOT IN ('ended','failed');",
            {sessionId})) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE media_sessions SET state='failed', terminal_reason=?, ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE session_id=? AND state NOT IN ('ended','failed','expired','revoked');";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool bound = bindText(statement, 1, reasonCode) && bindText(statement, 2, sessionId);
    const int result = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);
    return result == SQLITE_DONE && changed == 1 && transaction.commit();
}

bool MediaSessionRepository::endBundle(
    const std::string& sessionId,
    const std::string& reasonCode)
{
    if (!safeIdentifier(sessionId) || !safeIdentifier(reasonCode)) {
        return false;
    }
    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) {
        return false;
    }

    if (!executeBound(
            database_,
            "UPDATE media_access_grants SET active=0, revoked_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND revoked_at='';",
            {sessionId}) ||
        !executeBound(
            database_,
            "UPDATE media_provider_stream_leases SET state='ended', ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
            "WHERE route_id=(SELECT route_id FROM media_routes WHERE session_id=?) AND state NOT IN ('ended','failed');",
            {sessionId}) ||
        !executeBound(
            database_,
            "UPDATE media_routes SET state='ended', updated_at=CURRENT_TIMESTAMP "
            "WHERE session_id=? AND state NOT IN ('ended','failed');",
            {sessionId})) {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE media_sessions SET state='ended', terminal_reason=?, ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE session_id=? AND state NOT IN ('ended','failed','expired','revoked');";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }
    const bool bound = bindText(statement, 1, reasonCode) && bindText(statement, 2, sessionId);
    const int result = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    const int changed = sqlite3_changes(database_.handle());
    sqlite3_finalize(statement);
    return result == SQLITE_DONE && changed == 1 && transaction.commit();
}

std::optional<StoredMediaSession> MediaSessionRepository::findSession(
    const std::string& sessionId) const
{
    if (!safeIdentifier(sessionId)) {
        return std::nullopt;
    }
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT session_id, actor_id, backend_id, resource_kind, resource_id, state, presentation_profile_id, terminal_reason "
        "FROM media_sessions WHERE session_id=?;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, sessionId)) {
        if (statement) sqlite3_finalize(statement);
        return std::nullopt;
    }
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }
    StoredMediaSession value;
    value.sessionId = columnText(statement, 0);
    value.actorId = columnText(statement, 1);
    value.backendId = columnText(statement, 2);
    value.resourceKind = columnText(statement, 3);
    value.resourceId = columnText(statement, 4);
    value.state = columnText(statement, 5);
    value.presentationProfileId = columnText(statement, 6);
    value.terminalReason = columnText(statement, 7);
    sqlite3_finalize(statement);
    return value;
}

std::optional<StoredMediaAccessGrant> MediaSessionRepository::findResolvedGrant(
    const std::string& grantId,
    int idleTimeoutSeconds) const
{
    if (!safeIdentifier(grantId) || idleTimeoutSeconds < 0 || idleTimeoutSeconds > 86400) {
        return std::nullopt;
    }
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT grant_id, session_id, route_id, route_epoch, actor_id, secret_hash, expires_at, last_seen_at, active, "
        "CASE WHEN expires_at<=CURRENT_TIMESTAMP THEN 1 ELSE 0 END, "
        "CASE WHEN ?1>0 AND last_seen_at<=datetime('now','-' || ?1 || ' seconds') THEN 1 ELSE 0 END, "
        "CASE WHEN revoked_at<>'' THEN 1 ELSE 0 END "
        "FROM media_access_grants WHERE grant_id=?2;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        sqlite3_bind_int(statement, 1, idleTimeoutSeconds) != SQLITE_OK ||
        !bindText(statement, 2, grantId)) {
        if (statement) sqlite3_finalize(statement);
        return std::nullopt;
    }
    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        return std::nullopt;
    }
    StoredMediaAccessGrant value;
    value.grantId = columnText(statement, 0);
    value.sessionId = columnText(statement, 1);
    value.routeId = columnText(statement, 2);
    value.routeEpoch = sqlite3_column_int64(statement, 3);
    value.actorId = columnText(statement, 4);
    value.secretHash = columnText(statement, 5);
    value.expiresAt = columnText(statement, 6);
    value.lastSeenAt = columnText(statement, 7);
    value.active = sqlite3_column_int(statement, 8) != 0;
    value.expired = sqlite3_column_int(statement, 9) != 0;
    value.idleExpired = sqlite3_column_int(statement, 10) != 0;
    value.revoked = sqlite3_column_int(statement, 11) != 0;
    sqlite3_finalize(statement);
    return value;
}

std::optional<bool> MediaSessionRepository::touchGrantIfDue(
    const std::string& grantId,
    int minimumIntervalSeconds) const
{
    if (!safeIdentifier(grantId) || minimumIntervalSeconds <= 0 || minimumIntervalSeconds > 3600) {
        return std::nullopt;
    }
    auto lease = database_.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE media_access_grants SET last_seen_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE grant_id=?1 AND active=1 AND revoked_at='' AND expires_at>CURRENT_TIMESTAMP "
        "AND last_seen_at<=datetime('now','-' || ?2 || ' seconds');";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK ||
        !bindText(statement, 1, grantId) ||
        sqlite3_bind_int(statement, 2, minimumIntervalSeconds) != SQLITE_OK) {
        if (statement) sqlite3_finalize(statement);
        return std::nullopt;
    }
    const int result = sqlite3_step(statement);
    sqlite3_finalize(statement);
    if (result != SQLITE_DONE) {
        return std::nullopt;
    }
    return true;
}

bool MediaSessionRepository::recoverNonTerminalBundles()
{
    auto lease = database_.acquireTransactionLease();
    DatabaseTransaction transaction(database_);
    if (!transaction.active()) {
        return false;
    }

    return database_.execute(
        "UPDATE media_access_grants SET active=0, revoked_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE active=1 OR revoked_at='';") &&
        database_.execute(
        "UPDATE media_provider_stream_leases SET state='failed', ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE state IN ('provisioning','active','draining');") &&
        database_.execute(
        "UPDATE media_routes SET state='failed', updated_at=CURRENT_TIMESTAMP "
        "WHERE state IN ('provisioning','ready','active','draining');") &&
        database_.execute(
        "UPDATE media_sessions SET state='failed', terminal_reason='daemon_restart_ownership_lost', "
        "ended_at=CURRENT_TIMESTAMP, updated_at=CURRENT_TIMESTAMP "
        "WHERE state IN ('requested','authorized','provisioning','ready','active','draining');") &&
        transaction.commit();
}

bool MediaSessionRepository::supportsSecretHash(const std::string& secretHash)
{
    if (secretHash.rfind("$6$rounds=10000$", 0) != 0 || secretHash.size() > 256) {
        return false;
    }
    return std::none_of(
        secretHash.begin(), secretHash.end(),
        [](unsigned char character) {
            return character == '\0' || character == '\r' || character == '\n' ||
                std::iscntrl(character);
        });
}
