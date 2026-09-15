#include "EmbeddedRecordingCutRepository.h"

#include <sqlite3.h>

#include <memory>

namespace
{
using Statement =
    std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

Statement prepare(Database& database, const char* sql)
{
    sqlite3_stmt* value = nullptr;
    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &value,
            nullptr) != SQLITE_OK)
    {
        sqlite3_finalize(value);
        value = nullptr;
    }

    return Statement(value, sqlite3_finalize);
}

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

std::string text(sqlite3_stmt* statement, int index)
{
    const auto* value =
        sqlite3_column_text(statement, index);

    return value
        ? reinterpret_cast<const char*>(value)
        : std::string();
}
}

bool EmbeddedRecordingCutRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS embedded_recording_cut_commands ("
        "backend_id TEXT NOT NULL,"
        "operation_id TEXT NOT NULL,"
        "request_identity TEXT NOT NULL,"
        "command_id TEXT NOT NULL UNIQUE,"
        "fingerprint TEXT NOT NULL,"
        "payload TEXT NOT NULL,"
        "instance_id TEXT NOT NULL,"
        "state TEXT NOT NULL,"
        "evidence TEXT NOT NULL DEFAULT '',"
        "edited_recording_key TEXT NOT NULL DEFAULT '',"
        "PRIMARY KEY(backend_id,operation_id));");
}

bool EmbeddedRecordingCutRepository::find(
    const std::string& backendId,
    const std::string& operationId,
    EmbeddedRecordingCutRecord& record)
{
    record = {};
    record.backendId = backendId;
    record.operationId = operationId;

    auto lease = database_.acquireTransactionLease();

    auto row = prepare(
        database_,
        "SELECT request_identity,command_id,fingerprint,payload,"
        "instance_id,state,evidence,edited_recording_key "
        "FROM embedded_recording_cut_commands "
        "WHERE backend_id=? AND operation_id=?;");

    if (!row ||
        !bindText(row.get(), 1, backendId) ||
        !bindText(row.get(), 2, operationId))
    {
        return false;
    }

    const int step = sqlite3_step(row.get());

    if (step == SQLITE_DONE)
        return true;

    if (step != SQLITE_ROW)
        return false;

    record.found = true;
    record.requestIdentity = text(row.get(), 0);
    record.commandId = text(row.get(), 1);
    record.fingerprint = text(row.get(), 2);
    record.payload = text(row.get(), 3);
    record.instanceId = text(row.get(), 4);
    record.state = text(row.get(), 5);
    record.evidence = text(row.get(), 6);
    record.editedRecordingKey = text(row.get(), 7);

    return true;
}

bool EmbeddedRecordingCutRepository::insert(
    const EmbeddedRecordingCutRecord& record)
{
    auto lease = database_.acquireTransactionLease();

    auto row = prepare(
        database_,
        "INSERT INTO embedded_recording_cut_commands "
        "(backend_id,operation_id,request_identity,command_id,"
        "fingerprint,payload,instance_id,state) "
        "VALUES(?,?,?,?,?,?,?,'starting');");

    return row &&
        bindText(row.get(), 1, record.backendId) &&
        bindText(row.get(), 2, record.operationId) &&
        bindText(row.get(), 3, record.requestIdentity) &&
        bindText(row.get(), 4, record.commandId) &&
        bindText(row.get(), 5, record.fingerprint) &&
        bindText(row.get(), 6, record.payload) &&
        bindText(row.get(), 7, record.instanceId) &&
        sqlite3_step(row.get()) == SQLITE_DONE;
}

bool EmbeddedRecordingCutRepository::update(
    const EmbeddedRecordingCutRecord& record)
{
    auto lease = database_.acquireTransactionLease();

    auto row = prepare(
        database_,
        "UPDATE embedded_recording_cut_commands "
        "SET state=?,evidence=?,edited_recording_key=? "
        "WHERE backend_id=? AND operation_id=? "
        "AND command_id=? AND fingerprint=?;");

    return row &&
        bindText(row.get(), 1, record.state) &&
        bindText(row.get(), 2, record.evidence) &&
        bindText(row.get(), 3, record.editedRecordingKey) &&
        bindText(row.get(), 4, record.backendId) &&
        bindText(row.get(), 5, record.operationId) &&
        bindText(row.get(), 6, record.commandId) &&
        bindText(row.get(), 7, record.fingerprint) &&
        sqlite3_step(row.get()) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) == 1;
}
