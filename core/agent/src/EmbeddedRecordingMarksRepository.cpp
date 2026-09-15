#include "EmbeddedRecordingMarksRepository.h"
#include <sqlite3.h>
#include <memory>

namespace
{
using Statement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;
Statement prepare(Database& db, const char* sql)
{
    sqlite3_stmt* value = nullptr;
    if (sqlite3_prepare_v2(db.handle(), sql, -1, &value, nullptr) != SQLITE_OK)
    { sqlite3_finalize(value); value = nullptr; }
    return Statement(value, sqlite3_finalize);
}
bool bindText(sqlite3_stmt* stmt, int index, const std::string& value)
{
    return sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}
std::string text(sqlite3_stmt* stmt, int index)
{
    const auto* value = sqlite3_column_text(stmt, index);
    return value ? reinterpret_cast<const char*>(value) : "";
}
}
bool EmbeddedRecordingMarksRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS embedded_recording_marks_commands ("
        "backend_id TEXT NOT NULL,operation_id TEXT NOT NULL,request_identity TEXT NOT NULL,"
        "command_id TEXT NOT NULL UNIQUE,fingerprint TEXT NOT NULL,payload TEXT NOT NULL,"
        "instance_id TEXT NOT NULL,state TEXT NOT NULL,evidence TEXT NOT NULL DEFAULT '',"
        "canonical_revision TEXT NOT NULL DEFAULT '',PRIMARY KEY(backend_id,operation_id));");
}
bool EmbeddedRecordingMarksRepository::find(const std::string& backendId,
    const std::string& operationId, EmbeddedRecordingMarksRecord& r)
{
    r = {}; r.backendId = backendId; r.operationId = operationId;
    auto lease = database_.acquireTransactionLease();
    auto row = prepare(database_, "SELECT request_identity,command_id,fingerprint,payload,"
        "instance_id,state,evidence,canonical_revision FROM embedded_recording_marks_commands "
        "WHERE backend_id=? AND operation_id=?;");
    if (!row || !bindText(row.get(), 1, backendId) || !bindText(row.get(), 2, operationId)) return false;
    const int step = sqlite3_step(row.get());
    if (step == SQLITE_DONE) return true;
    if (step != SQLITE_ROW) return false;
    r.found = true;
    r.requestIdentity = text(row.get(), 0); r.commandId = text(row.get(), 1);
    r.fingerprint = text(row.get(), 2); r.payload = text(row.get(), 3);
    r.instanceId = text(row.get(), 4); r.state = text(row.get(), 5);
    r.evidence = text(row.get(), 6); r.canonicalRevision = text(row.get(), 7);
    return true;
}
bool EmbeddedRecordingMarksRepository::insert(const EmbeddedRecordingMarksRecord& r)
{
    auto lease = database_.acquireTransactionLease();
    auto row = prepare(database_, "INSERT INTO embedded_recording_marks_commands "
        "(backend_id,operation_id,request_identity,command_id,fingerprint,payload,instance_id,state) "
        "VALUES(?,?,?,?,?,?,?,'starting');");
    return row && bindText(row.get(), 1, r.backendId) && bindText(row.get(), 2, r.operationId) &&
        bindText(row.get(), 3, r.requestIdentity) && bindText(row.get(), 4, r.commandId) &&
        bindText(row.get(), 5, r.fingerprint) && bindText(row.get(), 6, r.payload) &&
        bindText(row.get(), 7, r.instanceId) && sqlite3_step(row.get()) == SQLITE_DONE;
}
bool EmbeddedRecordingMarksRepository::update(const EmbeddedRecordingMarksRecord& r)
{
    auto lease = database_.acquireTransactionLease();
    auto row = prepare(database_, "UPDATE embedded_recording_marks_commands SET state=?,evidence=?,"
        "canonical_revision=? WHERE backend_id=? AND operation_id=? AND command_id=? AND fingerprint=?;");
    return row && bindText(row.get(), 1, r.state) && bindText(row.get(), 2, r.evidence) &&
        bindText(row.get(), 3, r.canonicalRevision) && bindText(row.get(), 4, r.backendId) &&
        bindText(row.get(), 5, r.operationId) && bindText(row.get(), 6, r.commandId) &&
        bindText(row.get(), 7, r.fingerprint) && sqlite3_step(row.get()) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) == 1;
}
