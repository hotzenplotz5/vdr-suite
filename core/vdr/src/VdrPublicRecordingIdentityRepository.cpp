#include "VdrPublicRecordingIdentityRepository.h"

#include "Database.h"

#include <sqlite3.h>

#include <map>
#include <set>
#include <string>
#include <utility>

namespace
{
struct Statement
{
    sqlite3_stmt* value = nullptr;
    Statement(sqlite3* database, const char* sql)
    {
        if (sqlite3_prepare_v2(database, sql, -1, &value, nullptr) != SQLITE_OK)
        {
            value = nullptr;
        }
    }
    ~Statement() { sqlite3_finalize(value); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    explicit operator bool() const { return value != nullptr; }
    void bind(int position, const std::string& text)
    {
        sqlite3_bind_text(value, position, text.c_str(), -1, SQLITE_TRANSIENT);
    }
};

std::string textColumn(sqlite3_stmt* statement, int column)
{
    const auto* text = sqlite3_column_text(statement, column);
    return text ? reinterpret_cast<const char*>(text) : std::string{};
}

std::string sourceAddress(const VdrRecording& recording)
{
    return recording.backendNativeId.empty()
        ? recording.path : recording.backendNativeId;
}
}

VdrPublicRecordingIdentityRepository::VdrPublicRecordingIdentityRepository(
    Database& database)
    : database_(database)
{
}

bool VdrPublicRecordingIdentityRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS vdr_public_recording_identity ("
        "public_id TEXT PRIMARY KEY,"
        "backend_id TEXT NOT NULL,"
        "source_address TEXT NOT NULL,"
        "start_time TEXT NOT NULL,"
        "title TEXT NOT NULL DEFAULT '',"
        "active INTEGER NOT NULL DEFAULT 1,"
        "updated_at TEXT DEFAULT CURRENT_TIMESTAMP"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_vdr_public_recording_active_source "
        "ON vdr_public_recording_identity (backend_id, source_address) WHERE active = 1;"
        "CREATE INDEX IF NOT EXISTS idx_vdr_public_recording_backend "
        "ON vdr_public_recording_identity (backend_id, active, public_id);");
}

bool VdrPublicRecordingIdentityRepository::reconcileBackend(
    const std::string& backendId,
    const std::vector<VdrRecording>& recordings)
{
    if (backendId.empty() || !ensureSchema()) return false;

    std::set<std::string> addresses;
    for (const auto& recording : recordings)
    {
        if (!recording.backendId.empty() && recording.backendId != backendId)
            return false;
        const std::string address = sourceAddress(recording);
        // Incomplete inputs are not granted a public identity.
        if (address.empty() || recording.startTime.empty()) continue;
        if (!addresses.insert(address).second) return false;
    }

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    const auto rollback = [this]() { database_.execute("ROLLBACK;"); };

    std::map<std::string, std::pair<std::string, std::string>> existing;
    {
        Statement select(database_.handle(),
            "SELECT source_address, public_id, start_time "
            "FROM vdr_public_recording_identity "
            "WHERE backend_id = ? AND active = 1;");
        if (!select) { rollback(); return false; }
        select.bind(1, backendId);
        int step = SQLITE_ROW;
        while ((step = sqlite3_step(select.value)) == SQLITE_ROW)
        {
            existing.emplace(textColumn(select.value, 0),
                std::make_pair(textColumn(select.value, 1),
                               textColumn(select.value, 2)));
        }
        if (step != SQLITE_DONE) { rollback(); return false; }
    }

    {
        Statement deactivate(database_.handle(),
            "UPDATE vdr_public_recording_identity SET active=0 "
            "WHERE backend_id = ? AND active = 1;");
        if (!deactivate) { rollback(); return false; }
        deactivate.bind(1, backendId);
        if (sqlite3_step(deactivate.value) != SQLITE_DONE)
        { rollback(); return false; }
    }

    Statement restore(database_.handle(),
        "UPDATE vdr_public_recording_identity "
        "SET active=1, title=?, updated_at=CURRENT_TIMESTAMP "
        "WHERE public_id=? AND backend_id=? AND active=0;");
    Statement insert(database_.handle(),
        "INSERT INTO vdr_public_recording_identity "
        "(public_id,backend_id,source_address,start_time,title,active) "
        "VALUES ('rec_' || lower(hex(randomblob(16))), ?, ?, ?, ?, 1);");
    if (!restore || !insert) { rollback(); return false; }

    for (const auto& recording : recordings)
    {
        const std::string address = sourceAddress(recording);
        if (address.empty() || recording.startTime.empty()) continue;
        const auto previous = existing.find(address);
        const bool sameSourceAndStart = previous != existing.end() &&
            previous->second.second == recording.startTime;

        if (sameSourceAndStart)
        {
            sqlite3_reset(restore.value);
            sqlite3_clear_bindings(restore.value);
            restore.bind(1, recording.title);
            restore.bind(2, previous->second.first);
            restore.bind(3, backendId);
            if (sqlite3_step(restore.value) != SQLITE_DONE ||
                sqlite3_changes(database_.handle()) != 1)
            { rollback(); return false; }
        }
        else
        {
            sqlite3_reset(insert.value);
            sqlite3_clear_bindings(insert.value);
            insert.bind(1, backendId);
            insert.bind(2, address);
            insert.bind(3, recording.startTime);
            insert.bind(4, recording.title);
            if (sqlite3_step(insert.value) != SQLITE_DONE)
            { rollback(); return false; }
        }
    }

    if (!database_.execute("COMMIT;"))
    { rollback(); return false; }
    return true;
}

std::vector<VdrPublicRecordingBinding>
VdrPublicRecordingIdentityRepository::activeBindingsForBackend(
    const std::string& backendId) const
{
    std::vector<VdrPublicRecordingBinding> result;
    if (backendId.empty()) return result;
    auto lease = database_.acquireTransactionLease();
    Statement query(database_.handle(),
        "SELECT public_id,backend_id,source_address "
        "FROM vdr_public_recording_identity "
        "WHERE backend_id = ? AND active=1 ORDER BY public_id;");
    if (!query) return {};
    query.bind(1, backendId);
    int step = SQLITE_ROW;
    while ((step = sqlite3_step(query.value)) == SQLITE_ROW)
        result.push_back({textColumn(query.value, 0),
                          textColumn(query.value, 1),
                          textColumn(query.value, 2)});
    return step == SQLITE_DONE ? result : std::vector<VdrPublicRecordingBinding>{};
}

std::optional<VdrPublicRecordingBinding>
VdrPublicRecordingIdentityRepository::findActiveBinding(
    const std::string& publicRecordingId) const
{
    if (publicRecordingId.empty()) return std::nullopt;
    auto lease = database_.acquireTransactionLease();
    Statement query(database_.handle(),
        "SELECT public_id,backend_id,source_address "
        "FROM vdr_public_recording_identity WHERE public_id=? AND active=1;");
    if (!query) return std::nullopt;
    query.bind(1, publicRecordingId);
    if (sqlite3_step(query.value) != SQLITE_ROW) return std::nullopt;
    return VdrPublicRecordingBinding{textColumn(query.value, 0),
        textColumn(query.value, 1), textColumn(query.value, 2)};
}

bool VdrPublicRecordingIdentityRepository::rebindVerifiedMove(
    const std::string& publicRecordingId,
    const std::string& backendId,
    const std::string& expectedSourceAddress,
    const std::string& newSourceAddress)
{
    if (publicRecordingId.empty() || backendId.empty() ||
        expectedSourceAddress.empty() || newSourceAddress.empty() ||
        expectedSourceAddress == newSourceAddress)
        return false;

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    Statement update(database_.handle(),
        "UPDATE vdr_public_recording_identity "
        "SET source_address=?, updated_at=CURRENT_TIMESTAMP "
        "WHERE public_id=? AND backend_id=? AND source_address=? AND active=1;");
    if (!update) { database_.execute("ROLLBACK;"); return false; }
    update.bind(1, newSourceAddress);
    update.bind(2, publicRecordingId);
    update.bind(3, backendId);
    update.bind(4, expectedSourceAddress);
    if (sqlite3_step(update.value) != SQLITE_DONE ||
        sqlite3_changes(database_.handle()) != 1)
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }
    return true;
}
