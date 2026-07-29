#include "AccountabilityEventRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <iostream>
#include <string>

namespace
{
bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
               statement,
               index,
               value.c_str(),
               -1,
               SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* text = sqlite3_column_text(statement, column);
    return text == nullptr
        ? std::string()
        : std::string(reinterpret_cast<const char*>(text));
}

void reportAppendFailure(
    const char* operation,
    sqlite3* database,
    int result)
{
    const int extendedResult = database == nullptr
        ? result
        : sqlite3_extended_errcode(database);

    std::cerr
        << "accountability append "
        << operation
        << " failed: sqlite_rc="
        << result
        << ", sqlite_extended_rc="
        << extendedResult
        << std::endl;
}
}

AccountabilityEventRepository::AccountabilityEventRepository(Database& database)
    : database_(database)
{
}

bool AccountabilityEventRepository::ensureSchema()
{
    return database_.execute(
               "CREATE TABLE IF NOT EXISTS accountability_events ("
               "event_id TEXT PRIMARY KEY,"
               "schema_version INTEGER NOT NULL,"
               "classes TEXT NOT NULL,"
               "event_type TEXT NOT NULL,"
               "severity TEXT NOT NULL,"
               "occurred_at TEXT NOT NULL,"
               "recorded_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
               "actor_id TEXT NOT NULL,"
               "actor_type TEXT NOT NULL,"
               "device_id TEXT NOT NULL,"
               "session_id TEXT NOT NULL,"
               "authentication_state TEXT NOT NULL,"
               "permission TEXT NOT NULL,"
               "backend_id TEXT NOT NULL,"
               "operation_id TEXT NOT NULL,"
               "request_id TEXT NOT NULL,"
               "correlation_id TEXT NOT NULL,"
               "action TEXT NOT NULL,"
               "decision TEXT NOT NULL,"
               "reason_code TEXT NOT NULL,"
               "outcome TEXT NOT NULL"
               ");") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS "
               "idx_accountability_events_request "
               "ON accountability_events(request_id, event_id);") &&
        database_.execute(
               "CREATE INDEX IF NOT EXISTS "
               "idx_accountability_events_actor "
               "ON accountability_events(actor_id, event_id);") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "accountability_events_no_update "
               "BEFORE UPDATE ON accountability_events "
               "BEGIN "
               "SELECT RAISE(ABORT, "
               "'accountability events are append-only'); "
               "END;") &&
        database_.execute(
               "CREATE TRIGGER IF NOT EXISTS "
               "accountability_events_no_delete "
               "BEFORE DELETE ON accountability_events "
               "BEGIN "
               "SELECT RAISE(ABORT, "
               "'accountability events are append-only'); "
               "END;");
}

bool AccountabilityEventRepository::append(
    const AccountabilityEvent& event)
{
    if (event.eventId.empty() ||
        event.eventType.empty() ||
        event.requestId.empty())
    {
        return false;
    }

    auto transactionLease = database_.acquireTransactionLease();
    sqlite3* database = database_.handle();
    if (database == nullptr)
    {
        reportAppendFailure("database", nullptr, SQLITE_MISUSE);
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO accountability_events ("
        "event_id, schema_version, classes, event_type, severity, "
        "occurred_at, actor_id, actor_type, device_id, session_id, "
        "authentication_state, permission, backend_id, operation_id, "
        "request_id, correlation_id, action, decision, reason_code, outcome"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    const int prepareResult = sqlite3_prepare_v2(
        database,
        sql,
        -1,
        &statement,
        nullptr);
    if (prepareResult != SQLITE_OK)
    {
        reportAppendFailure("prepare", database, prepareResult);
        return false;
    }

    bool bound = true;
    bound = bindText(statement, 1, event.eventId) && bound;
    bound = sqlite3_bind_int(
        statement,
        2,
        event.schemaVersion) == SQLITE_OK && bound;
    bound = bindText(statement, 3, event.classes) && bound;
    bound = bindText(statement, 4, event.eventType) && bound;
    bound = bindText(statement, 5, event.severity) && bound;
    bound = bindText(statement, 6, event.occurredAt) && bound;
    bound = bindText(statement, 7, event.actorId) && bound;
    bound = bindText(statement, 8, event.actorType) && bound;
    bound = bindText(statement, 9, event.deviceId) && bound;
    bound = bindText(statement, 10, event.sessionId) && bound;
    bound = bindText(statement, 11, event.authenticationState) && bound;
    bound = bindText(statement, 12, event.permission) && bound;
    bound = bindText(statement, 13, event.backendId) && bound;
    bound = bindText(statement, 14, event.operationId) && bound;
    bound = bindText(statement, 15, event.requestId) && bound;
    bound = bindText(statement, 16, event.correlationId) && bound;
    bound = bindText(statement, 17, event.action) && bound;
    bound = bindText(statement, 18, event.decision) && bound;
    bound = bindText(statement, 19, event.reasonCode) && bound;
    bound = bindText(statement, 20, event.outcome) && bound;

    if (!bound)
    {
        reportAppendFailure(
            "bind",
            database,
            sqlite3_errcode(database));
        sqlite3_finalize(statement);
        return false;
    }

    const int stepResult = sqlite3_step(statement);
    if (stepResult != SQLITE_DONE)
    {
        reportAppendFailure("step", database, stepResult);
        sqlite3_finalize(statement);
        return false;
    }

    const int finalizeResult = sqlite3_finalize(statement);
    if (finalizeResult != SQLITE_OK)
    {
        reportAppendFailure("finalize", database, finalizeResult);
        return false;
    }

    return true;
}

std::vector<AccountabilityEvent> AccountabilityEventRepository::listAll() const
{
    std::vector<AccountabilityEvent> events;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT event_id, schema_version, classes, event_type, severity, "
        "occurred_at, actor_id, actor_type, device_id, session_id, "
        "authentication_state, permission, backend_id, operation_id, "
        "request_id, correlation_id, action, decision, reason_code, outcome "
        "FROM accountability_events "
        "ORDER BY recorded_at, event_id;";

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return events;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        AccountabilityEvent event;
        event.eventId = columnText(statement, 0);
        event.schemaVersion = sqlite3_column_int(statement, 1);
        event.classes = columnText(statement, 2);
        event.eventType = columnText(statement, 3);
        event.severity = columnText(statement, 4);
        event.occurredAt = columnText(statement, 5);
        event.actorId = columnText(statement, 6);
        event.actorType = columnText(statement, 7);
        event.deviceId = columnText(statement, 8);
        event.sessionId = columnText(statement, 9);
        event.authenticationState = columnText(statement, 10);
        event.permission = columnText(statement, 11);
        event.backendId = columnText(statement, 12);
        event.operationId = columnText(statement, 13);
        event.requestId = columnText(statement, 14);
        event.correlationId = columnText(statement, 15);
        event.action = columnText(statement, 16);
        event.decision = columnText(statement, 17);
        event.reasonCode = columnText(statement, 18);
        event.outcome = columnText(statement, 19);
        events.push_back(event);
    }

    sqlite3_finalize(statement);
    return events;
}
