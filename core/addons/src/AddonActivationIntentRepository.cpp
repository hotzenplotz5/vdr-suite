#include "AddonActivationIntentRepository.h"
#include "AddonCatalog.h"

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
                c == '-' || c == '_' || c == '.' || c == ':';
        });
}

bool validDigest(const std::string& sha)
{
    return sha.size() == 64 &&
        std::all_of(sha.begin(), sha.end(), [](unsigned char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
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


enum class MaintenanceActivationGate { unfrozen, frozen, unavailable };

MaintenanceActivationGate activationMaintenanceGate(Database& db)
{
    // Read under the caller's BEGIN IMMEDIATE transaction: a concurrent
    // freeze/ownership transition and an admin desired-enable write cannot
    // cross without serializing. The missing table is NOT "unfrozen".
    Statement query(db.handle(),
        "SELECT frozen,revision FROM addon_media_import_maintenance "
        "WHERE module_id='rectools';");
    if (!query.get()) return MaintenanceActivationGate::unavailable;
    const int first = sqlite3_step(query.get());
    if (first == SQLITE_DONE) return MaintenanceActivationGate::unfrozen;
    if (first != SQLITE_ROW ||
        sqlite3_column_type(query.get(), 0) != SQLITE_INTEGER ||
        sqlite3_column_type(query.get(), 1) != SQLITE_INTEGER)
        return MaintenanceActivationGate::unavailable;
    const int frozen = sqlite3_column_int(query.get(), 0);
    const auto revision = sqlite3_column_int64(query.get(), 1);
    if ((frozen != 0 && frozen != 1) || revision <= 0 ||
        sqlite3_step(query.get()) != SQLITE_DONE)
        return MaintenanceActivationGate::unavailable;
    return frozen ? MaintenanceActivationGate::frozen :
                    MaintenanceActivationGate::unfrozen;
}

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
    return {status, {moduleId, backendId, false, 0, 0, {}, {}, {}}};
}
}

bool AddonActivationIntentRepository::validModule(const std::string& id)
{
    return reviewedAddon(id) != nullptr;
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
        ");") &&
        // Separate binding table avoids silently treating pre-upgrade
        // desiredEnabled rows as permits for a newly enrolled Agent.
        database_.execute(
        "CREATE TABLE IF NOT EXISTS addon_activation_agent_bindings ("
        "module_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision>0),"
        "backend_generation INTEGER NOT NULL CHECK(backend_generation>0),"
        "agent_id TEXT NOT NULL,"
        "agent_instance_id TEXT NOT NULL,"
        "PRIMARY KEY(module_id,backend_id)"
        ");") &&
        // New independent pin table means legacy Agent-bound intents
        // cannot become executable after upgrades without admin reapproval.
        database_.execute(
        "CREATE TABLE IF NOT EXISTS addon_activation_handler_pins ("
        "module_id TEXT NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "intent_revision INTEGER NOT NULL CHECK(intent_revision>0),"
        "approved_sha256 TEXT NOT NULL,"
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
    if (result.intent.desiredEnabled) {
        // A legacy row without an exact same-revision Agent binding is only
        // an administrator wish, NEVER effective runtime authorization.
        Statement binding(database_.handle(),
            "SELECT intent_revision,backend_generation,agent_id,"
            "agent_instance_id FROM addon_activation_agent_bindings "
            "WHERE module_id=? AND backend_id=?;");
        if (binding.get() &&
            bindText(binding.get(), 1, moduleId) &&
            bindText(binding.get(), 2, backendId) &&
            sqlite3_step(binding.get()) == SQLITE_ROW) {
            const auto boundRevision = sqlite3_column_int64(binding.get(), 0);
            const auto boundGeneration = sqlite3_column_int64(binding.get(), 1);
            const auto* id = sqlite3_column_text(binding.get(), 2);
            const auto idBytes = sqlite3_column_bytes(binding.get(), 2);
            const auto* instance = sqlite3_column_text(binding.get(), 3);
            const auto instanceBytes = sqlite3_column_bytes(binding.get(), 3);
            if (boundRevision == revision && boundGeneration > 0 &&
                sqlite3_column_type(binding.get(), 0) == SQLITE_INTEGER &&
                sqlite3_column_type(binding.get(), 1) == SQLITE_INTEGER &&
                id != nullptr && idBytes > 0 && idBytes <= 128 &&
                instance != nullptr &&
                instanceBytes > 0 && instanceBytes <= 128) {
                // Never truncate at an embedded NUL: a forged TEXT value
                // must not pass as a valid Agent identity prefix.
                const std::string agent(
                    reinterpret_cast<const char*>(id),
                    static_cast<std::size_t>(idBytes));
                const std::string epoch(
                    reinterpret_cast<const char*>(instance),
                    static_cast<std::size_t>(instanceBytes));
                if (safeToken(agent, 128) && agent != "*" &&
                    safeToken(epoch, 128) && epoch != "*" &&
                    sqlite3_step(binding.get()) == SQLITE_DONE) {
                    Statement pin(database_.handle(),
                        "SELECT intent_revision,approved_sha256 "
                        "FROM addon_activation_handler_pins "
                        "WHERE module_id=? AND backend_id=?;");
                    if (pin.get() && bindText(pin.get(), 1, moduleId) &&
                        bindText(pin.get(), 2, backendId) &&
                        sqlite3_step(pin.get()) == SQLITE_ROW &&
                        sqlite3_column_type(pin.get(), 0) == SQLITE_INTEGER &&
                        sqlite3_column_int64(pin.get(), 0) == revision &&
                        sqlite3_column_type(pin.get(), 1) == SQLITE_TEXT &&
                        sqlite3_column_bytes(pin.get(), 1) == 64) {
                        const auto* chars = sqlite3_column_text(pin.get(), 1);
                        const std::string sha(
                            reinterpret_cast<const char*>(chars), 64);
                        if (validDigest(sha) &&
                            sqlite3_step(pin.get()) == SQLITE_DONE) {
                            result.intent.backendGeneration =
                                static_cast<std::uint64_t>(boundGeneration);
                            result.intent.agentId = agent;
                            result.intent.agentInstanceId = epoch;
                            result.intent.approvedHandlerSha256 = sha;
                        }
                    }
                }
            }
        }
    }
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

IntentResult AddonActivationIntentRepository::readForAdmission(
    const std::string& moduleId, const std::string& backendId) const
{
    if (!validModule(moduleId) || !validBackend(backendId))
        return outcome(IntentStatus::invalid, moduleId, backendId);
    auto lease = database_.acquireTransactionLease();
    return readInTransaction(moduleId, backendId);
}

IntentCatalogResult AddonActivationIntentRepository::readCatalog(
    const RequestSecurityContext& actor, const std::string& backendId) const
{
    IntentCatalogResult result;
    if (!canInspectInventory(actor).allowed)
    {
        result.status = IntentStatus::forbidden;
        return result;
    }
    if (!validBackend(backendId))
    {
        result.status = IntentStatus::invalid;
        return result;
    }
    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN;"))
        return result;
    // BEGIN gives all three SELECTs a coherent SQLite read snapshot.
    // Any invalid/unavailable row fails without returning partial results.
    for (const auto& definition : kReviewedAddonCatalog)
    {
        const IntentResult row = readInTransaction(
            std::string(definition.id), backendId);
        if (row.status != IntentStatus::ok)
        {
            database_.execute("ROLLBACK;");
            result.intents.clear();
            return result;
        }
        result.intents.push_back(row.intent);
    }
    if (!database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        result.intents.clear();
        return result;
    }
    result.status = IntentStatus::ok;
    return result;
}

bool AddonActivationIntentRepository::eligibleForFutureEnable(
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence)
{
    // Enabling is an audited administrator preference. It must not require
    // a currently leased Agent or a freshly published installation snapshot.
    // An offline enable stores no executable provider binding. The actual
    // operation still requires a live, independently verified provider.
    if (mutation.moduleId != "rectools" ||
        backend.backendId != mutation.backendId ||
        !backend.backendFound || !backend.allowed || backend.readOnly)
        return false;
    const bool unbound =
        evidence.moduleId.empty() && evidence.packageName.empty() &&
        evidence.backendId.empty() && evidence.agentId.empty() &&
        evidence.agentInstanceId.empty() && evidence.backendGeneration == 0 &&
        evidence.approvedHandlerSha256.empty() && !evidence.installed &&
        !evidence.manifestValid && !evidence.packageTrusted &&
        !evidence.versionCompatible && !evidence.handlerRegistered &&
        !evidence.handlerHealthy;
    if (unbound) return true;
    return evidence.moduleId == "rectools" &&
        evidence.packageName == "vdr-suite-addon-media-tools" &&
        evidence.backendId == mutation.backendId &&
        safeToken(evidence.agentId, 128) && evidence.agentId != "*" &&
        safeToken(evidence.agentInstanceId, 128) &&
        evidence.agentInstanceId != "*" &&
        validDigest(evidence.approvedHandlerSha256) &&
        evidence.backendGeneration > 0 &&
        evidence.backendGeneration <=
            static_cast<std::uint64_t>(
                std::numeric_limits<std::int64_t>::max()) &&
        evidence.installed && evidence.manifestValid && evidence.packageTrusted &&
        evidence.versionCompatible && evidence.handlerRegistered &&
        evidence.handlerHealthy;
}

IntentResult AddonActivationIntentRepository::update(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence)
{
    return updateInternal(actor, mutation, backend, evidence, nullptr);
}

IntentResult AddonActivationIntentRepository::updateAudited(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence,
    const std::function<bool()>& appendAudit)
{
    if (!appendAudit)
        return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);
    return updateInternal(actor, mutation, backend, evidence, &appendAudit);
}

IntentResult AddonActivationIntentRepository::updateInternal(
    const RequestSecurityContext& actor,
    const IntentMutation& mutation,
    const BackendAccessDecision& backend,
    const RuntimeEvidence& evidence,
    const std::function<bool()>* appendAudit)
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

    if (mutation.desiredEnabled) {
        // A package freeze owns the WHOLE Media Tools activation boundary,
        // not just new Import/HEVC reservations. Disabling still works
        // during maintenance, even with missing handler or backend facts.
        const auto gate = activationMaintenanceGate(database_);
        if (gate == MaintenanceActivationGate::frozen)
            return outcome(IntentStatus::ineligible, mutation.moduleId, mutation.backendId);
        if (gate != MaintenanceActivationGate::unfrozen)
            return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);
    }

    // An already persisted exact state at the caller's CURRENT revision is
    // a no-op. Do not increment the revision or append another audit event.
    // Revision 0 is special: an explicit first disable must still create a
    // durable disabled row (and its audit record) for removal/maintenance.
    // Stale revisions always conflict, even when the boolean happens to
    // match, so an intervening enable/disable cannot be mistaken for replay.
    if (current.intent.revision > 0 &&
        current.intent.desiredEnabled == mutation.desiredEnabled &&
        (!mutation.desiredEnabled || evidence.agentId.empty() ||
         (current.intent.backendGeneration == evidence.backendGeneration &&
          current.intent.agentId == evidence.agentId &&
          current.intent.agentInstanceId == evidence.agentInstanceId &&
          current.intent.approvedHandlerSha256 ==
              evidence.approvedHandlerSha256)))
        return current;

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
    // Preserve a verified handler pin when one is available. Offline
    // administrative enables intentionally have no provider binding.
    if (mutation.desiredEnabled && !evidence.agentId.empty()) {
        Statement bound(database_.handle(),
            "INSERT INTO addon_activation_agent_bindings "
            "(module_id,backend_id,intent_revision,backend_generation,"
            "agent_id,agent_instance_id) VALUES (?,?,?,?,?,?) "
            "ON CONFLICT(module_id,backend_id) DO UPDATE SET "
            "intent_revision=excluded.intent_revision,"
            "backend_generation=excluded.backend_generation,"
            "agent_id=excluded.agent_id,"
            "agent_instance_id=excluded.agent_instance_id;");
        if (!bound.get() ||
            !bindText(bound.get(), 1, mutation.moduleId) ||
            !bindText(bound.get(), 2, mutation.backendId) ||
            sqlite3_bind_int64(bound.get(), 3, nextRevision) != SQLITE_OK ||
            sqlite3_bind_int64(bound.get(), 4,
                static_cast<std::int64_t>(evidence.backendGeneration)) != SQLITE_OK ||
            !bindText(bound.get(), 5, evidence.agentId) ||
            !bindText(bound.get(), 6, evidence.agentInstanceId) ||
            sqlite3_step(bound.get()) != SQLITE_DONE)
            return outcome(IntentStatus::unavailable,
                           mutation.moduleId, mutation.backendId);
        Statement pin(database_.handle(),
            "INSERT INTO addon_activation_handler_pins "
            "(module_id,backend_id,intent_revision,approved_sha256) "
            "VALUES (?,?,?,?) ON CONFLICT(module_id,backend_id) "
            "DO UPDATE SET intent_revision=excluded.intent_revision,"
            "approved_sha256=excluded.approved_sha256;");
        if (!pin.get() ||
            !bindText(pin.get(), 1, mutation.moduleId) ||
            !bindText(pin.get(), 2, mutation.backendId) ||
            sqlite3_bind_int64(pin.get(), 3, nextRevision) != SQLITE_OK ||
            !bindText(pin.get(), 4, evidence.approvedHandlerSha256) ||
            sqlite3_step(pin.get()) != SQLITE_DONE)
            return outcome(IntentStatus::unavailable,
                           mutation.moduleId, mutation.backendId);
    } else {
        Statement clear(database_.handle(),
            "DELETE FROM addon_activation_agent_bindings "
            "WHERE module_id=? AND backend_id=?;");
        if (!clear.get() ||
            !bindText(clear.get(), 1, mutation.moduleId) ||
            !bindText(clear.get(), 2, mutation.backendId) ||
            sqlite3_step(clear.get()) != SQLITE_DONE)
            return outcome(IntentStatus::unavailable,
                           mutation.moduleId, mutation.backendId);
        Statement clearPin(database_.handle(),
            "DELETE FROM addon_activation_handler_pins "
            "WHERE module_id=? AND backend_id=?;");
        if (!clearPin.get() ||
            !bindText(clearPin.get(), 1, mutation.moduleId) ||
            !bindText(clearPin.get(), 2, mutation.backendId) ||
            sqlite3_step(clearPin.get()) != SQLITE_DONE)
            return outcome(IntentStatus::unavailable,
                           mutation.moduleId, mutation.backendId);
    }
    // Same DB transaction as the desired-state write: a failed accountability
    // append must rollback both state and audit. The callback must use the
    // existing Suite AccountabilityEventRepository on this same Database.
    if (appendAudit != nullptr)
    {
        try
        {
            if (!(*appendAudit)())
                return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);
        }
        catch (...)
        {
            return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);
        }
    }
    if (!tx.commit())
        return outcome(IntentStatus::unavailable, mutation.moduleId, mutation.backendId);

    IntentResult updated = outcome(IntentStatus::ok, mutation.moduleId, mutation.backendId);
    updated.intent.desiredEnabled = mutation.desiredEnabled;
    updated.intent.revision = nextRevision;
    if (mutation.desiredEnabled) {
        updated.intent.backendGeneration = evidence.backendGeneration;
        updated.intent.agentId = evidence.agentId;
        updated.intent.agentInstanceId = evidence.agentInstanceId;
        updated.intent.approvedHandlerSha256 = evidence.approvedHandlerSha256;
    }
    return updated;
}

} // namespace vdrsuite::addons
