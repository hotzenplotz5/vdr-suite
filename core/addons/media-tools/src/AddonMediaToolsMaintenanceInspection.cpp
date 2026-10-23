#include "AddonMediaToolsMaintenanceInspection.h"

#include "MediaToolsImportRunLock.h"

#include <sqlite3.h>

#include <cstdint>
#include <optional>
#include <utility>

namespace vdrsuite::addons {
namespace {

struct ReadTransaction {
    explicit ReadTransaction(Database& db)
        : db_(db), active_(db.execute("BEGIN IMMEDIATE TRANSACTION;")) {}
    ~ReadTransaction()
    {
        if (active_) db_.execute("ROLLBACK;");
    }
    bool active() const { return active_; }
private:
    Database& db_;
    bool active_;
};

std::optional<std::int64_t> scalar(Database& database, const char* sql)
{
    sqlite3_stmt* s = nullptr;
    if (sqlite3_prepare_v2(database.handle(), sql, -1, &s, nullptr) != SQLITE_OK)
        return std::nullopt;
    const int first = sqlite3_step(s);
    std::optional<std::int64_t> result;
    if (first == SQLITE_ROW && sqlite3_column_type(s, 0) == SQLITE_INTEGER) {
        const auto number = sqlite3_column_int64(s, 0);
        if (number >= 0 && sqlite3_step(s) == SQLITE_DONE) result = number;
    }
    sqlite3_finalize(s);
    return result;
}

} // namespace

MaintenanceInspectionResult AddonMediaToolsMaintenanceInspection::inspect(
    const RequestSecurityContext& actor, const std::string& trustedRoot) const
{
    return inspectAndHold(actor, trustedRoot).inspection;
}

MaintenanceInspectionHold AddonMediaToolsMaintenanceInspection::inspectAndHold(
    const RequestSecurityContext& actor, const std::string& trustedRoot) const
{
    using Reason = MaintenanceInspectionReason;
    const auto deny = [](Reason reason, std::int64_t revision = 0)
        -> MaintenanceInspectionHold {
        return {{false, reason, revision}, std::nullopt};
    };
    if (actor.actor.type != ActorType::User ||
        !canInspectInventory(actor).allowed)
        return deny(Reason::forbidden, 0);
    if (intents_.databaseIdentity() != &database_ ||
        operations_.databaseIdentity() != &database_)
        return deny(Reason::incompatibleDatabases, 0);

    // Never follow the path chosen by package metadata or an HTTP request.
    // This path is supplied by the trusted Core install owner only.
    // Hold the EXCLUSIVE lock while reading the same durable Core state.
    // It is deliberately released on return: this CANNOT authorize upgrade.
    auto exclusive = vdrsuite::mediatools::MediaToolsImportRunLock::tryAcquire(
        trustedRoot,
        vdrsuite::mediatools::MediaToolsImportRunLock::Mode::exclusiveMaintenance);
    if (!exclusive)
        return deny(Reason::runLockUnavailable, 0);

    auto lease = database_.acquireTransactionLease();
    ReadTransaction transaction(database_);
    if (!transaction.active())
        return deny(Reason::storageUnavailable, 0);

    const auto frozen = scalar(database_,
        "SELECT COUNT(*) FROM addon_media_import_maintenance "
        "WHERE module_id='rectools' AND frozen=1 AND revision>0;");
    if (!frozen)
        return deny(Reason::storageUnavailable, 0);
    if (*frozen != 1)
        return deny(Reason::globalFreezeRequired, 0);
    const auto revision = scalar(database_,
        "SELECT revision FROM addon_media_import_maintenance "
        "WHERE module_id='rectools' AND frozen=1;");
    if (!revision || *revision <= 0)
        return deny(Reason::storageUnavailable, 0);

    // Inspect all recorded backends, never just a caller-supplied backend
    // that could hide an enabled row elsewhere. Require at least one
    // explicitly disabled, audited desired-state row.
    const auto disabled = scalar(database_,
        "SELECT COUNT(*) FROM addon_activation_intents "
        "WHERE module_id='rectools' AND desired_enabled=0 AND revision>0;");
    const auto enabled = scalar(database_,
        "SELECT COUNT(*) FROM addon_activation_intents "
        "WHERE module_id='rectools' AND "
        "(desired_enabled<>0 OR revision<=0);");
    if (!disabled || !enabled)
        return deny(Reason::storageUnavailable, *revision);
    if (*enabled != 0)
        return deny(Reason::activationStillEnabled, *revision);
    if (*disabled == 0)
        return deny(Reason::auditedDisableMissing, *revision);

    // Inspect EVERY Media Tools mutation family, not just Import. The
    // same installed package also owns HEVC Stage/Publish executors; even
    // an otherwise drained Import worker cannot make an active Shrink safe
    // for package replacement. This unbounded query cannot hide work behind
    // a saturated 256-operation listing.
    const auto unresolved = scalar(database_,
        "SELECT COUNT(*) FROM mutation_operations "
        "WHERE action_family IN ("
        "'media.import','media.shrink.stage','media.shrink.publish') "
        "AND state NOT IN ("
        "'rejected','conflict','succeeded','failed_before_dispatch',"
        "'failed_verified','cancelled');");
    if (!unresolved)
        return deny(Reason::storageUnavailable, *revision);
    if (*unresolved != 0)
        return deny(Reason::operationsUnresolved, *revision);

    // Durable Agent assignments are an INDEPENDENT authority from operations.
    // Expired or waiting-reconciliation commands are NOT proof of a drained
    // handler. Only completed commands with a persisted result qualify.
    const auto pendingCommands = scalar(database_,
        "SELECT COUNT(*) FROM backend_agent_commands "
        "WHERE command_type IN ("
        "'addon.media.import','addon.media.shrink.stage',"
        "'addon.media.shrink.publish') AND state<>'completed';");
    if (!pendingCommands)
        return deny(Reason::storageUnavailable, *revision);
    if (*pendingCommands != 0)
        return deny(Reason::agentCommandsUnresolved, *revision);

    const auto missingResults = scalar(database_,
        "SELECT COUNT(*) FROM backend_agent_commands AS c "
        "LEFT JOIN backend_agent_command_results AS r "
        "ON r.command_id=c.command_id "
        "WHERE c.command_type IN ("
        "'addon.media.import','addon.media.shrink.stage',"
        "'addon.media.shrink.publish') "
        "AND c.state='completed' AND r.command_id IS NULL;");
    if (!missingResults)
        return deny(Reason::storageUnavailable, *revision);
    if (*missingResults != 0)
        return deny(Reason::agentResultsMissing, *revision);

    // An Agent that still publishes this optional executor may immediately
    // re-enter it on new dispatch. Require its advertised capability removed
    // before even returning a locally quiescent diagnostic snapshot.
    const auto advertised = scalar(database_,
        "SELECT COUNT(*) FROM backend_agent_command_capabilities "
        "WHERE command_type IN ("
        "'addon.media.import','addon.media.shrink.stage',"
        "'addon.media.shrink.publish');");
    if (!advertised)
        return deny(Reason::storageUnavailable, *revision);
    if (*advertised != 0)
        return deny(Reason::agentCapabilityStillAdvertised, *revision);

    // Unlike inspect(), the successful result retains the EXACT protected
    // inode lock until the move-only hold is destroyed. This only proves
    // worker exclusion while held, not stable SQL authority or package trust.
    return {{true, Reason::locallyQuiescentSnapshot, *revision},
            std::move(exclusive)};
}

} // namespace vdrsuite::addons
