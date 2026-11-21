#include "AddonMediaImportMaintenanceFence.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"
#include "MediaToolsImportRunLock.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>
#include <array>
#include <cerrno>
#include <sys/random.h>

namespace vdrsuite::addons {
namespace {

bool safeToken(const std::string& value)
{
    return !value.empty() && value.size() <= 128 &&
        std::all_of(value.begin(), value.end(), [](unsigned char ch) {
            return std::isalnum(ch) || ch == '-' || ch == '_' ||
                ch == '.' || ch == ':';
        });
}

bool validUtc(const std::string& t)
{
    if (t.size() != 20 || t[4] != '-' || t[7] != '-' ||
        t[10] != 'T' || t[13] != ':' || t[16] != ':' || t[19] != 'Z')
        return false;
    for (std::size_t i = 0; i < t.size(); ++i)
        if (i != 4 && i != 7 && i != 10 && i != 13 &&
            i != 16 && i != 19 && !std::isdigit(
                static_cast<unsigned char>(t[i]))) return false;
    const auto num = [&](std::size_t i, std::size_t count) {
        int v = 0;
        for (std::size_t n = 0; n < count; ++n) v = v * 10 + (t[i+n] - '0');
        return v;
    };
    const int year = num(0, 4), month = num(5, 2), day = num(8, 2);
    if (year < 1 || month < 1 || month > 12 || num(11, 2) > 23 ||
        num(14, 2) > 59 || num(17, 2) > 59) return false;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    const int days[] = {31, leap ? 29 : 28, 31, 30, 31, 30,
                        31, 31, 30, 31, 30, 31};
    return day > 0 && day <= days[month-1];
}

bool administrator(const RequestSecurityContext& actor)
{
    return actor.actor.type == ActorType::User &&
        canInspectInventory(actor).allowed;
}

// Called with an optional SQLite write lease already held.
MaintenanceFenceState readRow(Database& db)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT frozen,revision FROM addon_media_import_maintenance "
        "WHERE module_id='rectools';";
    if (sqlite3_prepare_v2(db.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return {};
    MaintenanceFenceState result;
    const int first = sqlite3_step(statement);
    if (first == SQLITE_DONE)
        result = {MaintenanceFenceStatus::ok, false, 0};
    else if (first == SQLITE_ROW)
    {
        const int frozen = sqlite3_column_int(statement, 0);
        const auto revision = sqlite3_column_int64(statement, 1);
        if ((frozen == 0 || frozen == 1) && revision > 0 &&
            sqlite3_step(statement) == SQLITE_DONE)
            result = {MaintenanceFenceStatus::ok, frozen == 1, revision};
    }
    sqlite3_finalize(statement);
    if (result.status != MaintenanceFenceStatus::ok) return result;

    sqlite3_stmt* owner = nullptr;
    if (sqlite3_prepare_v2(db.handle(),
            "SELECT owner_actor_id,owner_token,claimed_revision "
            "FROM addon_media_import_maintenance_owners "
            "WHERE module_id='rectools';", -1, &owner, nullptr) != SQLITE_OK)
    {
        if (owner) sqlite3_finalize(owner);
        return {};
    }
    const int present = sqlite3_step(owner);
    if (present == SQLITE_ROW)
    {
        const unsigned char* actor = sqlite3_column_text(owner, 0);
        const unsigned char* token = sqlite3_column_text(owner, 1);
        const auto claimed = sqlite3_column_int64(owner, 2);
        if (actor != nullptr && token != nullptr && claimed > 0 &&
            result.frozen && result.revision >= claimed &&
            sqlite3_column_bytes(owner, 1) == 32 &&
            sqlite3_step(owner) == SQLITE_DONE)
            result.ownershipActive = true;
        else result = {};
    }
    else if (present != SQLITE_DONE) result = {};
    sqlite3_finalize(owner);
    return result;
}

bool validContext(const RequestSecurityContext& actor,
                  const MaintenanceFenceAuditContext& context)
{
    return safeToken(actor.actor.actorId) &&
           safeToken(actor.requestId) && safeToken(actor.correlationId) &&
           safeToken(context.eventId) && validUtc(context.occurredAt);
}

bool appendMaintenanceAudit(
    AccountabilityEventRepository& audit,
    const RequestSecurityContext& actor,
    const MaintenanceFenceAuditContext& context,
    const char* action, const char* reason)
{
    AccountabilityEvent event;
    event.eventId = context.eventId;
    event.classes = "security,administration";
    event.eventType = "addon.media-import.maintenance";
    event.severity = "info";
    event.occurredAt = context.occurredAt;
    event.actorId = actor.actor.actorId;
    event.actorType = actorTypeName(actor.actor.type);
    event.deviceId = actor.device ? actor.device->deviceId : "";
    event.sessionId = actor.session ? actor.session->sessionId : "";
    event.authenticationState =
        authenticationStateName(actor.authenticationState);
    event.permission = "role.admin";
    event.backendId = "";
    event.requestId = actor.requestId;
    event.correlationId = actor.correlationId;
    event.action = action;
    event.decision = "allowed";
    event.reasonCode = reason;
    event.outcome = "committed";
    return audit.append(event);
}

bool validOpaqueToken(const std::string& token)
{
    return token.size() == 32 &&
           std::all_of(token.begin(), token.end(), [](unsigned char c) {
               return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
           });
}

std::string randomOwnerToken()
{
    std::array<unsigned char, 16> bytes{};
    std::size_t pos = 0;
    while (pos != bytes.size())
    {
        const ssize_t got = ::getrandom(bytes.data() + pos,
                                       bytes.size() - pos, GRND_NONBLOCK);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) return {};
        pos += static_cast<std::size_t>(got);
    }
    constexpr char digits[] = "0123456789abcdef";
    std::string token;
    token.reserve(32);
    for (const unsigned char byte : bytes)
    {
        token.push_back(digits[byte >> 4]);
        token.push_back(digits[byte & 15]);
    }
    return token;
}

bool changeRevision(Database& db, std::int64_t expectedRevision,
                    const std::string& actor)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE addon_media_import_maintenance "
        "SET revision=?,updated_by_actor_id=? "
        "WHERE module_id='rectools' AND frozen=1 AND revision=?;";
    bool ok = sqlite3_prepare_v2(db.handle(), sql, -1, &statement,
                                 nullptr) == SQLITE_OK;
    ok = ok &&
        sqlite3_bind_int64(statement, 1, expectedRevision + 1) == SQLITE_OK &&
        sqlite3_bind_text(statement, 2, actor.c_str(), -1,
                          SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_int64(statement, 3, expectedRevision) == SQLITE_OK &&
        sqlite3_step(statement) == SQLITE_DONE &&
        sqlite3_changes(db.handle()) == 1;
    sqlite3_finalize(statement);
    return ok;
}

struct WriteTransaction {
    explicit WriteTransaction(Database& db)
        : db(db), active(db.execute("BEGIN IMMEDIATE TRANSACTION;")) {}
    ~WriteTransaction() { if (active) db.execute("ROLLBACK;"); }
    bool commit()
    {
        if (!active || !db.execute("COMMIT;")) return false;
        active = false;
        return true;
    }
    Database& db;
    bool active;
};

} // namespace

MaintenanceFenceState AddonMediaImportMaintenanceFence::read(
    const RequestSecurityContext& actor) const
{
    if (!administrator(actor) ||
        operations_.databaseIdentity() != &db_ ||
        audit_.databaseIdentity() != &db_)
        return {MaintenanceFenceStatus::forbidden, false, 0};
    auto lease = db_.acquireTransactionLease();
    return readRow(db_);
}

MaintenanceFenceState AddonMediaImportMaintenanceFence::setFrozen(
    const RequestSecurityContext& actor, bool frozen,
    std::int64_t expectedRevision,
    const MaintenanceFenceAuditContext& context)
{
    if (!administrator(actor) ||
        operations_.databaseIdentity() != &db_ ||
        audit_.databaseIdentity() != &db_ ||
        !safeToken(actor.actor.actorId) ||
        !safeToken(actor.requestId) || !safeToken(actor.correlationId))
        return {MaintenanceFenceStatus::forbidden, false, 0};
    if (expectedRevision < 0 ||
        expectedRevision == std::numeric_limits<std::int64_t>::max() ||
        !validContext(actor, context))
        return {MaintenanceFenceStatus::invalid, false, 0};

    auto lease = db_.acquireTransactionLease();
    WriteTransaction transaction(db_);
    if (!transaction.active) return {};
    const auto existing = readRow(db_);
    if (existing.status != MaintenanceFenceStatus::ok)
        return {};
    if (existing.ownershipActive)
        return {MaintenanceFenceStatus::ownershipActive, existing.frozen,
                existing.revision, true};
    if (existing.revision != expectedRevision)
        return {MaintenanceFenceStatus::revisionConflict,
                existing.frozen, existing.revision};

    const std::int64_t newRevision = expectedRevision + 1;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO addon_media_import_maintenance"
        "(module_id,frozen,revision,updated_by_actor_id) VALUES('rectools',?,?,?) "
        "ON CONFLICT(module_id) DO UPDATE SET "
        "frozen=excluded.frozen,revision=excluded.revision,"
        "updated_by_actor_id=excluded.updated_by_actor_id;";
    bool ok = sqlite3_prepare_v2(db_.handle(), sql, -1, &stmt, nullptr) == SQLITE_OK;
    ok = ok && sqlite3_bind_int(stmt, 1, frozen ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int64(stmt, 2, newRevision) == SQLITE_OK &&
        sqlite3_bind_text(stmt, 3, actor.actor.actorId.c_str(),
                          -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) return {};

    if (!appendMaintenanceAudit(
            audit_, actor, context,
            frozen ? "addons.maintenance.freeze" : "addons.maintenance.unfreeze",
            "admin_maintenance_gate") ||
        !transaction.commit()) return {};
    return {MaintenanceFenceStatus::ok, frozen, newRevision};
}


MaintenanceOwnershipResult AddonMediaImportMaintenanceFence::claimOwnership(
    const RequestSecurityContext& actor,
    std::int64_t expectedRevision,
    const vdrsuite::mediatools::MediaToolsImportRunLock& exclusive,
    const MaintenanceFenceAuditContext& context)
{
    if (!administrator(actor) ||
        operations_.databaseIdentity() != &db_ ||
        audit_.databaseIdentity() != &db_ ||
        !exclusive.ownsExclusiveLock() || !validContext(actor, context))
        return {{MaintenanceFenceStatus::forbidden, false, 0}, {}};
    if (expectedRevision <= 0 ||
        expectedRevision == std::numeric_limits<std::int64_t>::max())
        return {{MaintenanceFenceStatus::invalid, false, 0}, {}};

    const std::string token = randomOwnerToken();
    if (!validOpaqueToken(token)) return {};

    auto lease = db_.acquireTransactionLease();
    WriteTransaction transaction(db_);
    if (!transaction.active) return {};
    const auto current = readRow(db_);
    if (current.status != MaintenanceFenceStatus::ok) return {};
    if (current.ownershipActive)
        return {{MaintenanceFenceStatus::ownershipActive, true,
                 current.revision, true}, {}};
    if (current.revision != expectedRevision)
        return {{MaintenanceFenceStatus::revisionConflict, current.frozen,
                 current.revision}, {}};
    if (!current.frozen)
        return {{MaintenanceFenceStatus::freezeRequired, false,
                 current.revision}, {}};
    if (!changeRevision(db_, expectedRevision, actor.actor.actorId))
        return {};

    sqlite3_stmt* insert = nullptr;
    const char* sql =
        "INSERT INTO addon_media_import_maintenance_owners "
        "(module_id,owner_actor_id,owner_token,claimed_revision) "
        "VALUES('rectools',?,?,?);";
    bool ok = sqlite3_prepare_v2(db_.handle(), sql, -1,
                                 &insert, nullptr) == SQLITE_OK;
    ok = ok &&
        sqlite3_bind_text(insert, 1, actor.actor.actorId.c_str(),
                          -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(insert, 2, token.c_str(),
                          -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_int64(insert, 3, expectedRevision + 1) == SQLITE_OK &&
        sqlite3_step(insert) == SQLITE_DONE;
    sqlite3_finalize(insert);
    if (!ok || !appendMaintenanceAudit(
            audit_, actor, context, "addons.maintenance.owner.claim",
            "exclusive_maintenance_ownership") ||
        !transaction.commit()) return {};
    return {{MaintenanceFenceStatus::ok, true,
             expectedRevision + 1, true}, token};
}

MaintenanceFenceState AddonMediaImportMaintenanceFence::releaseOwnership(
    const RequestSecurityContext& actor,
    std::int64_t expectedRevision, const std::string& token,
    const vdrsuite::mediatools::MediaToolsImportRunLock& exclusive,
    const MaintenanceFenceAuditContext& context)
{
    if (!administrator(actor) ||
        operations_.databaseIdentity() != &db_ ||
        audit_.databaseIdentity() != &db_ ||
        !exclusive.ownsExclusiveLock() || !validContext(actor, context))
        return {MaintenanceFenceStatus::forbidden, false, 0};
    if (expectedRevision <= 0 ||
        expectedRevision == std::numeric_limits<std::int64_t>::max() ||
        !validOpaqueToken(token))
        return {MaintenanceFenceStatus::invalid, false, 0};

    auto lease = db_.acquireTransactionLease();
    WriteTransaction transaction(db_);
    if (!transaction.active) return {};
    const auto current = readRow(db_);
    if (current.status != MaintenanceFenceStatus::ok) return {};
    if (current.revision != expectedRevision)
        return {MaintenanceFenceStatus::revisionConflict,
                current.frozen, current.revision, current.ownershipActive};
    if (!current.frozen || !current.ownershipActive)
        return {MaintenanceFenceStatus::ownershipMismatch,
                current.frozen, current.revision, current.ownershipActive};

    sqlite3_stmt* remove = nullptr;
    const char* sql =
        "DELETE FROM addon_media_import_maintenance_owners "
        "WHERE module_id='rectools' AND owner_actor_id=? AND owner_token=?;";
    bool ok = sqlite3_prepare_v2(db_.handle(), sql, -1,
                                 &remove, nullptr) == SQLITE_OK;
    ok = ok &&
        sqlite3_bind_text(remove, 1, actor.actor.actorId.c_str(),
                          -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_bind_text(remove, 2, token.c_str(),
                          -1, SQLITE_TRANSIENT) == SQLITE_OK &&
        sqlite3_step(remove) == SQLITE_DONE &&
        sqlite3_changes(db_.handle()) == 1;
    sqlite3_finalize(remove);
    if (!ok)
        return {MaintenanceFenceStatus::ownershipMismatch,
                true, current.revision, true};
    if (!changeRevision(db_, expectedRevision, actor.actor.actorId) ||
        !appendMaintenanceAudit(
            audit_, actor, context, "addons.maintenance.owner.release",
            "exclusive_maintenance_ownership") ||
        !transaction.commit()) return {};
    // Ownership ends, but the global admission freeze is intentionally
    // NOT lifted. It must be lifted by a separate audited admin operation.
    return {MaintenanceFenceStatus::ok, true, expectedRevision + 1, false};
}


} // namespace vdrsuite::addons
