#include "AddonMediaImportMaintenanceFence.h"

#include "AccountabilityEvent.h"
#include "AccountabilityEventRepository.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <limits>

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
    return result;
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
        !safeToken(context.eventId) || !validUtc(context.occurredAt))
        return {MaintenanceFenceStatus::invalid, false, 0};

    auto lease = db_.acquireTransactionLease();
    WriteTransaction transaction(db_);
    if (!transaction.active) return {};
    const auto existing = readRow(db_);
    if (existing.status != MaintenanceFenceStatus::ok)
        return {};
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
    event.action = frozen ? "addons.maintenance.freeze" :
                            "addons.maintenance.unfreeze";
    event.decision = "allowed";
    event.reasonCode = "admin_maintenance_gate";
    event.outcome = "committed";

    if (!audit_.append(event) || !transaction.commit()) return {};
    return {MaintenanceFenceStatus::ok, frozen, newRevision};
}

} // namespace vdrsuite::addons
