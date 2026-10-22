#include "PublicSearchTimerIdentityRepository.h"

#include "Database.h"
#include "SearchTimer.h"

#include <sqlite3.h>

#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace
{
struct Binding
{
    std::string publicId;
    std::string signature;
};

bool bind(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(statement, index, value.data(),
        static_cast<int>(value.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

bool run(sqlite3* database, const char* sql,
    const std::vector<std::string>& values)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    bool ok = true;
    for (std::size_t i = 0U; i < values.size(); ++i)
        if (!bind(statement, static_cast<int>(i + 1U), values[i]))
            ok = false;
    if (ok) ok = sqlite3_step(statement) == SQLITE_DONE;
    const int finalize = sqlite3_finalize(statement);
    return ok && finalize == SQLITE_OK;
}

void field(std::string& signature, const std::string& value)
{
    signature += std::to_string(value.size()) + ":" + value;
}

void field(std::string& signature, int value)
{
    field(signature, std::to_string(value));
}

// Deliberately excludes the active/inactive state. A toggle is not a new
// search rule. Observable rule changes that cannot be proven to be an edit
// conservatively retire the old public ID rather than risk misdirected writes.
std::string fingerprint(const SearchTimer& timer)
{
    std::string signature = "searchtimer-observation-v1|";
    field(signature, timer.name());
    field(signature, timer.query());
    field(signature, timer.recordingOptions().directory());
    field(signature, timer.recordingOptions().priority());
    field(signature, timer.recordingOptions().lifetime());
    field(signature, timer.scheduleOptions().marginStartMinutes());
    field(signature, timer.scheduleOptions().marginStopMinutes());
    field(signature, timer.scheduleOptions().useVps());
    field(signature, timer.filterOptions().useChannel());
    field(signature, timer.filterOptions().useDayOfWeek());
    field(signature, timer.filterOptions().useDuration());
    field(signature, timer.filterOptions().durationMinMinutes());
    field(signature, timer.filterOptions().durationMaxMinutes());
    field(signature, timer.comparisonOptions().compareTitle());
    field(signature, timer.comparisonOptions().compareSubtitle());
    field(signature, timer.comparisonOptions().compareSummary());
    field(signature, timer.comparisonOptions().compareCategories());
    field(signature, timer.comparisonOptions().compareTime());
    field(signature, timer.repeatOptions().avoidRepeats());
    field(signature, timer.repeatOptions().allowedRepeats());
    field(signature, timer.repeatOptions().repeatsWithinDays());
    field(signature, timer.channelOptions().channels());
    field(signature, timer.channelOptions().channelMin());
    field(signature, timer.channelOptions().channelMax());
    field(signature, timer.seriesOptions().useSeriesRecording());
    field(signature, timer.seriesOptions().keepRecordings());
    field(signature, timer.seriesOptions().deleteMode());
    field(signature, timer.seriesOptions().searchTimerAction());
    field(signature, timer.blacklistOptions().blacklistMode());
    field(signature, timer.blacklistOptions().blacklistIds());
    field(signature, timer.matchOptions().mode());
    field(signature, timer.matchOptions().matchCase());
    field(signature, timer.matchOptions().tolerance());
    field(signature, timer.matchOptions().summaryMatch());
    field(signature, timer.extendedEpgOptions().useExtendedEpgInfo());
    field(signature, timer.extendedEpgOptions().extendedEpgInfo());
    field(signature, timer.extendedEpgOptions().ignoreMissingEpgCategories());
    field(signature, timer.extendedEpgOptions().contentDescriptors());
    field(signature, timer.validityOptions().useInFavorites());
    field(signature, timer.validityOptions().activeFrom());
    field(signature, timer.validityOptions().activeUntil());
    field(signature, timer.actionOptions().pauseOnRecordings());
    field(signature, timer.actionOptions().switchMinutesBefore());
    field(signature, timer.actionOptions().unmuteSoundOnSwitch());
    field(signature, timer.actionOptions().deleteRecordingsAfterDays());
    field(signature, timer.actionOptions().deleteAfterCountRecordings());
    field(signature, timer.actionOptions().deleteAfterDaysOfFirstRecording());
    return signature;
}

std::optional<std::map<std::string, Binding>> currentBindings(
    sqlite3* database, const std::string& backend)
{
    sqlite3_stmt* statement = nullptr;
    constexpr const char* sql =
        "SELECT i.native_id, i.public_id, COALESCE(s.signature, '') "
        "FROM public_search_timer_identity i "
        "LEFT JOIN public_search_timer_identity_signature s "
        "ON s.public_id=i.public_id WHERE i.backend_id=?1;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    std::map<std::string, Binding> rows;
    bool ok = bind(statement, 1, backend);
    int code = SQLITE_ERROR;
    if (ok)
    {
        while ((code = sqlite3_step(statement)) == SQLITE_ROW)
        {
            const auto* native = sqlite3_column_text(statement, 0);
            const auto* publicId = sqlite3_column_text(statement, 1);
            const auto* signature = sqlite3_column_text(statement, 2);
            if (!native || !publicId || !signature ||
                !rows.emplace(reinterpret_cast<const char*>(native),
                    Binding{reinterpret_cast<const char*>(publicId),
                        reinterpret_cast<const char*>(signature)}).second)
            {
                ok = false;
                break;
            }
        }
    }
    ok = ok && code == SQLITE_DONE;
    if (sqlite3_finalize(statement) != SQLITE_OK) ok = false;
    return ok ? std::optional<std::map<std::string, Binding>>(std::move(rows))
              : std::nullopt;
}

bool retire(sqlite3* database, const std::string& backend,
    const std::string& native, const Binding& previous)
{
    return run(database,
        "INSERT INTO public_search_timer_identity_retired "
        "(public_id, backend_id, native_id) VALUES(?1, ?2, ?3);",
        {previous.publicId, backend, native}) &&
        run(database,
            "DELETE FROM public_search_timer_identity_signature "
            "WHERE public_id=?1;", {previous.publicId}) &&
        run(database,
            "DELETE FROM public_search_timer_identity "
            "WHERE public_id=?1 AND backend_id=?2 AND native_id=?3;",
            {previous.publicId, backend, native});
}

std::optional<std::string> publicIdForNative(
    sqlite3* database, const std::string& backend, const std::string& native)
{
    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(database,
        "SELECT public_id FROM public_search_timer_identity "
        "WHERE backend_id=?1 AND native_id=?2;",
        -1, &statement, nullptr) != SQLITE_OK)
        return std::nullopt;
    std::optional<std::string> id;
    if (bind(statement, 1, backend) && bind(statement, 2, native) &&
        sqlite3_step(statement) == SQLITE_ROW)
    {
        const auto* raw = sqlite3_column_text(statement, 0);
        if (raw) id = reinterpret_cast<const char*>(raw);
    }
    if (sqlite3_finalize(statement) != SQLITE_OK) return std::nullopt;
    return id;
}

bool insertNew(sqlite3* database, const std::string& backend,
    const std::string& native, const std::string& signature,
    std::string& assigned)
{
    // Retired public IDs remain reserved. A hypothetical random collision
    // with a retired ID aborts the transaction rather than reusing it.
    constexpr const char* insert =
        "WITH candidate(id) AS (SELECT 'st_' || lower(hex(randomblob(16)))) "
        "INSERT INTO public_search_timer_identity "
        "(public_id, backend_id, native_id) "
        "SELECT id, ?1, ?2 FROM candidate WHERE NOT EXISTS "
        "(SELECT 1 FROM public_search_timer_identity_retired "
        "WHERE public_id=candidate.id);";
    if (!run(database, insert, {backend, native}) ||
        sqlite3_changes(database) != 1)
        return false;
    const auto id = publicIdForNative(database, backend, native);
    if (!id || !run(database,
        "INSERT INTO public_search_timer_identity_signature "
        "(public_id, signature) VALUES(?1, ?2);", {*id, signature}))
        return false;
    assigned = *id;
    return true;
}
} // namespace

PublicSearchTimerIdentityRepository::PublicSearchTimerIdentityRepository(
    Database& db)
    : database_(db)
{
}

bool PublicSearchTimerIdentityRepository::ensureSchema()
{
    auto lease = database_.acquireTransactionLease();
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS public_search_timer_identity ("
        "public_id TEXT PRIMARY KEY NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "native_id TEXT NOT NULL,"
        "UNIQUE (backend_id, native_id));") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS public_search_timer_identity_signature ("
        "public_id TEXT PRIMARY KEY NOT NULL,"
        "signature TEXT NOT NULL);") &&
        database_.execute(
        "CREATE TABLE IF NOT EXISTS public_search_timer_identity_retired ("
        "public_id TEXT PRIMARY KEY NOT NULL,"
        "backend_id TEXT NOT NULL,"
        "native_id TEXT NOT NULL);");
}

std::optional<std::vector<std::string>>
PublicSearchTimerIdentityRepository::reconcileAuthoritativeSnapshot(
    const std::string& backend, const std::vector<SearchTimer>& observations)
{
    if (backend.empty() || backend.size() > 128U ||
        observations.size() > 10000U) return std::nullopt;

    std::map<std::string, std::string> signatures;
    for (const auto& timer : observations)
    {
        const std::string& native = timer.backendNativeId();
        if (timer.backendId() != backend || native.empty() ||
            native.size() > 64U || timer.name().empty() ||
            timer.query().empty() || timer.name().size() > 1024U ||
            timer.query().size() > 1024U)
            return std::nullopt;
        std::string signature = fingerprint(timer);
        if (signature.size() > 16384U ||
            !signatures.emplace(native, std::move(signature)).second)
            return std::nullopt;
    }

    auto lease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE;")) return std::nullopt;
    bool success = false;
    std::vector<std::string> assigned;
    sqlite3* db = database_.handle();
    const auto old = currentBindings(db, backend);
    if (old)
    {
        success = true;
        for (const auto& entry : *old)
        {
            const auto observed = signatures.find(entry.first);
            // Legacy rows have no signature. Their provenance cannot be
            // established after an offline delete/recreate; rotate once.
            if (observed == signatures.end() ||
                entry.second.signature.empty() ||
                observed->second != entry.second.signature)
            {
                if (!retire(db, backend, entry.first, entry.second))
                {
                    success = false;
                    break;
                }
            }
        }

        if (success)
        {
            assigned.reserve(observations.size());
            for (const auto& timer : observations)
            {
                const auto existing = old->find(timer.backendNativeId());
                const auto observed = signatures.find(timer.backendNativeId());
                std::string id;
                if (existing != old->end() &&
                    !existing->second.signature.empty() &&
                    existing->second.signature == observed->second)
                    id = existing->second.publicId;
                else if (!insertNew(db, backend, timer.backendNativeId(),
                             observed->second, id))
                {
                    success = false;
                    break;
                }
                assigned.push_back(std::move(id));
            }
        }
    }

    if (success && database_.execute("COMMIT;"))
        return assigned;
    database_.execute("ROLLBACK;");
    return std::nullopt;
}
