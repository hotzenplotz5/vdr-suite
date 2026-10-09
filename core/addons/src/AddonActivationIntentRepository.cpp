#include "AddonActivationIntentRepository.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>

namespace vdrsuite::addons {
namespace {
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement, index, value.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK;
}

bool safeToken(const std::string& text, std::size_t maximum)
{
    return !text.empty() && text.size() <= maximum &&
        std::all_of(text.begin(), text.end(), [](unsigned char c) {
            return (c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9') ||
                c == '-' || c == '_' || c == '.';
        });
}

class Statement {
public:
    Statement(sqlite3* db, const char* sql)
    {
        if (db != nullptr &&
            sqlite3_prepare_v2(db, sql, -1, &statement_, nullptr) != SQLITE_OK)
            statement_ = nullptr;
    }
    ~Statement() { sqlite3_finalize(statement_); }
    sqlite3_stmt* get() const { return statement_; }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

private:
    sqlite3_stmt* statement_ = nullptr;
};

class Transaction {
public:
    explicit Transaction(Database& db)
        : db_(db), active_(db.execute("BEGIN IMMEDIATE;")) {}
    ~Transaction() { if (active_) db_.execute("ROLLBACK;"); }
    bool active() const { return active_; }
    bool commit()
    {
        if (!active_ || !db_.execute("COMMIT;")) return false;
        active_ = false;
        return true;
    }
private:
    Database& db_;
    bool active_;
};

IntentResult outcome(IntentStatus status,
                     const std::string& moduleId,
                     const std::string& backendId)
{
    return {status, {moduleId, backendId, false, 0}};
}
}

bool AddonActivationIntentRepository::validModule(const std::string& id)
{
    return id == "rectools" || id == "image" ||
        id == "music" || id == "tvscraper";
}

bool AddonActivationIntentRepository::validBackend(const std::string& id)
{
    return safeToken(id, 128) && id != "*";
}

bool AddonActivationIntentRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS addon_activation_intents ("
        "module_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "desired_enabled INTEGER NOT NULL CHECK(desired_enabled IN (0,1)),"
        "revision INTEGER NOT NULL CHECK(revision >= 1),"
        "updated_by_actor_id TEXT NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(module_id,backend_id)"
        ");");
}

IntentResult AddonActivationIntentRepository::readInTransaction(
    const std::string& moduleId, const std::string& backendId) const
{
    IntentResult result = outcome(IntentStatus::unavailable, moduleId, backendId);
    Statement query(database_.handle(),
        "SELECT desired_enabled, revision FROM addon_activation_intents "
        "WHERE module_id = ? AND backend_id = ?;");
    if (!query.get() ||
        !bindText(query.get(), 1, moduleId) ||
        !bindText(query.get(), 2, backendId))
        return result;

    const int rc = sqlite3_step(query.get());
    if (rc == SQLITE_DONE)
    {
        result.status = IntentStatus::ok;
        return result;
    }
    if (rc != SQLITE_ROW)
        return result;
    const auto enabled = sqlite3_column_int(query.get(), 0);
    const auto revision = sqlite3_column_int64(query.get(), 1);
    if ((enabled != 0 && enabled != 1) || revision <= 0 ||
        sqlite3_step(query.get()) != SQLITE_DONE)
        return result;
    result.intent.desiredEnabled = enabled == 1;
    result.intent.revision = revision;
    result.status = IntentStatus::ok;
    return result;
}

IntentResult AddonActivationIntentRepository::read(
    const RequestSecurityContext& actor,
    const std::string& moduleId,
    const std::string& backendId) const
{
    if (canInspectInventory(actor).allowed == false)
        return outcome(IntentStatus::forbidden, moduleId, backendId);
    if (!validModule(moduleId) || !validBackend(backendId))
        return outcome(IntentStatus::invalid, moduleId, backendId);
    auto lease = database_.acquireTransactionLease();
    return readInTransaction(moduleId, backendId);
}

bool AddonActivationIntentRepository::eligibleForFutureEnable(
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence)
{
    // No generic extension loader. Only audited Media Tools can ever be
    // considered here and only with all independent trusted evidence present.
    return mutation.moduleId == "rectools" &&
        evidence.moduleId == "rectools" &&
        evidence.packageName == "vdr-suite-addon-media-tools" &&
        evidence.backendId == mutation.backendId &&
        evidence.installed && evidence.manifestValid && evidence.packageTrusted &&
        evidence.versionCompatible && evidence.handlerRegistered &&
        evidence.handlerHealthy &&
        backend.backendId == mutation.backendId &&
        backend.backendFound && backend.allowed && !backend.readOnly;
}

IntentResult AddonActivationIntentRepository::update(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence)
{
    if (canInspectInventory(actor).allowed == false)
        return outcome(IntentStatus::forbidden, mutation.moduleId, mutation.backendId);
    if (!validModule(mutation.moduleId) || !validBackend(mutation.backendId) ||
        mutation.expectedRevision < 0 ||
        mutation.expectedRevision >= std::numeric_limits<std::int64_t>::max())
        return outcome(IntentStatus::invalid, mutation.moduleId, mutation.backendId);
    if (mutation.desiredEnabled &&
        !eligibleForFutureEnable(mutation, backend, evidence))
        return outcome(IntentStatus::ineligible, mutation.moduleId, mutation.backendId);

    auto lease = database_.acquireTransactionLease();
    Transaction tx(database_);
    if (!tx.active())
        return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);

    IntentResult current = readInTransaction(mutation.moduleId, mutation.backendId);
    if (current.status != IntentStatus::ok)
        return current;
    if (current.intent.revision != mutation.expectedRevision)
        return outcome(IntentStatus::revisionConflict, mutation.moduleId, mutation.backendId);

    // No variable SQL and no blind-upsert: the CAS is serialized inside the
    // same BEGIN IMMEDIATE transaction as the authoritative read.
    Statement write(database_.handle(),
        "INSERT INTO addon_activation_intents "
        "(module_id, backend_id, desired_enabled, revision, updated_by_actor_id) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(module_id, backend_id) DO UPDATE SET "
        "desired_enabled=excluded.desired_enabled,"
        "revision=excluded.revision,"
        "updated_by_actor_id=excluded.updated_by_actor_id,"
        "updated_at=CURRENT_TIMESTAMP;");
    const auto nextRevision = current.intent.revision + 1;
    if (!write.get() ||
        !bindText(write.get(), 1, mutation.moduleId) ||
        !bindText(write.get(), 2, mutation.backendId) ||
        sqlite3_bind_int(write.get(), 3, mutation.desiredEnabled ? 1 : 0) != SQLITE_OK ||
        sqlite3_bind_int64(write.get(), 4, nextRevision) != SQLITE_OK ||
        !bindText(write.get(), 5, actor.actor.actorId) ||
        sqlite3_step(write.get()) != SQLITE_DONE)
        return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);
    if (!tx.commit())
        return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);

    IntentResult updated = outcome(IntentStatus::ok, mutation.moduleId, mutation.backendId);
    updated.intent.desiredEnabled = mutation.desiredEnabled;
    updated.intent.revision = nextRevision;
    return updated;
}

} // namespace vdrsuite::addons
