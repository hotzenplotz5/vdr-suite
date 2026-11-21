#include "MediaImportCheckedPreviewStore.h"
#include "Database.h"

#include <sqlite3.h>
#include <algorithm>
#include <cstdint>

namespace media_import_preview_store {

bool ensureSchema(Database& db)
{
    return db.execute(
        "CREATE TABLE IF NOT EXISTS media_import_checked_previews ("
        "backend_id TEXT NOT NULL, source_id TEXT NOT NULL, "
        "payload BLOB NOT NULL, PRIMARY KEY(backend_id,source_id));");
}

bool load(Database& db, const std::string& backend,
          const std::string& sourceId, std::string& payload)
{
    payload.clear();
    auto lease = db.acquireTransactionLease();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(),
        "SELECT payload FROM media_import_checked_previews "
        "WHERE backend_id=? AND source_id=?", -1, &stmt, nullptr)
        != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const auto* blob = sqlite3_column_blob(stmt, 0);
        const int size = sqlite3_column_bytes(stmt, 0);
        if (blob && size > 0 && size <= 1000000) {
            payload.assign(static_cast<const char*>(blob),
                           static_cast<std::size_t>(size));
            found = true;
        }
    }
    sqlite3_finalize(stmt);
    return found;
}

void save(Database& db, const std::string& backend,
          const std::string& sourceId, const std::string& payload)
{
    if (payload.empty() || payload.size() > 1000000) return;
    auto lease = db.acquireTransactionLease();
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db.handle(),
        "INSERT OR REPLACE INTO media_import_checked_previews "
        "(backend_id,source_id,payload) VALUES (?,?,?)", -1,
        &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(stmt, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sourceId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(stmt, 3, payload.data(),
                      static_cast<int>(payload.size()), SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void prune(Database& db, const std::string& backend,
           const std::vector<std::string>& observed,
           const std::map<std::string, std::string>& statuses, bool partial)
{
    std::vector<std::string> retired;
    for (const auto& status : statuses)
        if (status.second == "recording_present")
            retired.push_back(status.first);
    auto lease = db.acquireTransactionLease();
    if (!partial) {
        sqlite3_stmt* query = nullptr;
        if (sqlite3_prepare_v2(db.handle(),
            "SELECT source_id FROM media_import_checked_previews "
            "WHERE backend_id=?", -1, &query, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(query, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
            while (sqlite3_step(query) == SQLITE_ROW) {
                const auto* value = sqlite3_column_text(query, 0);
                if (!value) continue;
                const std::string id(reinterpret_cast<const char*>(value));
                if (!std::binary_search(observed.begin(), observed.end(), id))
                    retired.push_back(id);
            }
        }
        sqlite3_finalize(query);
    }
    if (retired.empty()) return;
    sqlite3_stmt* remove = nullptr;
    if (sqlite3_prepare_v2(db.handle(),
        "DELETE FROM media_import_checked_previews "
        "WHERE backend_id=? AND source_id=?", -1, &remove, nullptr)
        != SQLITE_OK) return;
    for (const auto& id : retired) {
        sqlite3_reset(remove);
        sqlite3_clear_bindings(remove);
        sqlite3_bind_text(remove, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(remove, 2, id.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(remove) != SQLITE_DONE) break;
    }
    sqlite3_finalize(remove);
}

} // namespace media_import_preview_store
