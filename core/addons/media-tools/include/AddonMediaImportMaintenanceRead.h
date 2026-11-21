#pragma once

#include "Database.h"

#include <sqlite3.h>

namespace vdrsuite::addons {

// One common, fail-closed read of the authoritative admission freeze.
// Called by the server's final command receipt/capability revalidation.
// The canonical operation insertion still owns the atomic check inside its
// BEGIN IMMEDIATE transaction. This does not itself authorize a job.
inline bool mediaToolsAdmissionUnfrozen(Database& database)
{
    if (!database.isOpen()) return false;
    auto lease = database.acquireTransactionLease();
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database.handle(),
            "SELECT frozen,revision FROM addon_media_import_maintenance "
            "WHERE module_id='rectools';", -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const int first = sqlite3_step(statement);
    bool allowed = false;
    if (first == SQLITE_DONE)
        allowed = true;  // schema exists, no explicit freeze
    else if (first == SQLITE_ROW &&
             sqlite3_column_type(statement, 0) == SQLITE_INTEGER &&
             sqlite3_column_int(statement, 0) == 0 &&
             sqlite3_column_type(statement, 1) == SQLITE_INTEGER &&
             sqlite3_column_int64(statement, 1) > 0)
        allowed = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return allowed;
}

// Compatibility name for existing Import receipt checks.
inline bool mediaImportAdmissionUnfrozen(Database& database)
{
    return mediaToolsAdmissionUnfrozen(database);
}

} // namespace vdrsuite::addons
