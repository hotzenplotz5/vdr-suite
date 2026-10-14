// P7: exercise real C++ repository schema initializers on a disposable
// historical SQLite fixture. The caller controls a fresh private test path.
// NEVER use this helper against an installed VDR-Suite database.
#include "Database.h"
#include "GenreIndexRepository.h"
#include "GlobalSearchRepository.h"
#include <sqlite3.h>
#include <iostream>
#include <string>

namespace {
bool scalarPositive(sqlite3* db, const char* sql)
{
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    const bool ok = sqlite3_step(stmt) == SQLITE_ROW &&
                    sqlite3_column_int(stmt, 0) > 0;
    sqlite3_finalize(stmt);
    return ok;
}
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "expected one isolated SQLite fixture path\n";
        return 2;
    }
    Database database;
    if (!database.open(argv[1])) return 3;
    if (!database.execute("PRAGMA foreign_keys=ON;")) return 4;

    // New instances each pass defeat any in-memory schemaReady_ shortcut.
    for (int pass = 0; pass < 2; ++pass) {
        GenreIndexRepository genres(database);
        if (!genres.ensureSchema()) {
            std::cerr << "P7_METADATA_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 5;
        }
        GlobalSearchRepository globalSearch(database);
        if (!globalSearch.ensureSchema()) {
            std::cerr << "P7_GLOBAL_SEARCH_RUNTIME_SCHEMA=FAIL pass=" << pass << "\n";
            return 6;
        }
        if (!scalarPositive(database.handle(),
            "SELECT count(*) FROM suite_metadata_schema_versions;") ||
            !scalarPositive(database.handle(),
            "SELECT count(*) FROM vdr_global_search_schema_versions;") ||
            !scalarPositive(database.handle(),
            "SELECT count(*) FROM pragma_integrity_check WHERE integrity_check='ok';") ||
            !scalarPositive(database.handle(),
            "SELECT CASE WHEN (SELECT count(*) FROM pragma_foreign_key_check)=0 THEN 1 ELSE 0 END;")) {
            std::cerr << "P7_RUNTIME_SQLITE_INTEGRITY=FAIL\n";
            return 7;
        }
    }
    database.close();
    std::cout << "P7_RUNTIME_SCHEMA_REPOSITORIES=PASS\n";
    return 0;
}
