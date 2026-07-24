from .common import replace_once

# ---------------------------------------------------------------------------
# Shared SQLite connection: serialize explicit cache transactions process-wide.
# ---------------------------------------------------------------------------

replace_once(
    "core/sqlite/include/Database.h",
    '#include <string>\n',
    '#include <mutex>\n#include <string>\n'
)

replace_once(
    "core/sqlite/include/Database.h",
    '''class Database {
public:
    Database();
''',
    '''class Database {
public:
    using TransactionLease = std::unique_lock<std::recursive_mutex>;

    Database();
'''
)

replace_once(
    "core/sqlite/include/Database.h",
    '''    bool execute(const std::string& sql);
    bool tableExists(const std::string& tableName);
''',
    '''    bool execute(const std::string& sql);
    bool tableExists(const std::string& tableName);

    TransactionLease acquireTransactionLease();
'''
)

replace_once(
    "core/sqlite/include/Database.h",
    '''private:
    sqlite3* db_;
};
''',
    '''private:
    sqlite3* db_;
    mutable std::recursive_mutex transactionMutex_;
};
'''
)

replace_once(
    "core/sqlite/src/Database.cpp",
    '''sqlite3* Database::handle() const
{
    return db_;
}
''',
    '''Database::TransactionLease Database::acquireTransactionLease()
{
    return TransactionLease(transactionMutex_);
}

sqlite3* Database::handle() const
{
    return db_;
}
'''
)

replace_once(
    "core/vdr/src/VdrRecordingCacheRepository.cpp",
    '''    if (recordings.empty())
    {
        return true;
    }

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
''',
    '''    if (recordings.empty())
    {
        return true;
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
'''
)

replace_once(
    "core/vdr/src/VdrRecordingCacheRepository.cpp",
    '''    const std::string normalizedBackendId =
        normalizeBackendId(backendId);

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
''',
    '''    const std::string normalizedBackendId =
        normalizeBackendId(backendId);

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
'''
)

replace_once(
    "core/vdr/src/EpgEventRepository.cpp",
    '''    if (events.empty())
    {
        return true;
    }

    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
''',
    '''    if (events.empty())
    {
        return true;
    }

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
'''
)

replace_once(
    "core/vdr/src/EpgEventRepository.cpp",
    '''    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return result;
    }

    std::string selectSql =
''',
    '''    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;"))
    {
        return result;
    }

    std::string selectSql =
'''
)

