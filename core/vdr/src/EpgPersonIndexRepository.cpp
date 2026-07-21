#include "EpgPersonIndexRepository.h"

#include "Database.h"
#include "MetadataPlatformSchemaInstaller.h"
#include "PersonNameNormalizer.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <sqlite3.h>

namespace
{

bool bindText(
    sqlite3_stmt* statement,
    int index,
    const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value ? reinterpret_cast<const char*>(value) : std::string{};
}

long long epochSeconds()
{
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string roleName(PersonRole role)
{
    switch (role)
    {
    case PersonRole::Actor: return "actor";
    case PersonRole::Director: return "director";
    case PersonRole::Writer: return "writer";
    case PersonRole::Producer: return "producer";
    case PersonRole::Moderator: return "moderator";
    case PersonRole::Guest: return "guest";
    case PersonRole::Composer: return "composer";
    case PersonRole::Other: return "other";
    case PersonRole::Unknown: return "unknown";
    }
    return "unknown";
}

PersonRole roleFromName(const std::string& value)
{
    if (value == "actor") return PersonRole::Actor;
    if (value == "director") return PersonRole::Director;
    if (value == "writer") return PersonRole::Writer;
    if (value == "producer") return PersonRole::Producer;
    if (value == "moderator") return PersonRole::Moderator;
    if (value == "guest") return PersonRole::Guest;
    if (value == "composer") return PersonRole::Composer;
    if (value == "other") return PersonRole::Other;
    return PersonRole::Unknown;
}

PersonRole domainRole(EpgScraperPersonRole role)
{
    switch (role)
    {
    case EpgScraperPersonRole::Actor: return PersonRole::Actor;
    case EpgScraperPersonRole::Director: return PersonRole::Director;
    case EpgScraperPersonRole::Writer: return PersonRole::Writer;
    case EpgScraperPersonRole::Producer: return PersonRole::Producer;
    case EpgScraperPersonRole::Moderator: return PersonRole::Moderator;
    case EpgScraperPersonRole::Guest: return PersonRole::Guest;
    case EpgScraperPersonRole::Composer: return PersonRole::Composer;
    case EpgScraperPersonRole::Other: return PersonRole::Other;
    case EpgScraperPersonRole::Unknown: return PersonRole::Unknown;
    }
    return PersonRole::Unknown;
}

const char* mediaTypeName(EpgScraperMediaType type)
{
    switch (type)
    {
    case EpgScraperMediaType::Series: return "series";
    case EpgScraperMediaType::Movie: return "movie";
    case EpgScraperMediaType::None: return "none";
    }
    return "none";
}

std::string escapeJson(const std::string& value)
{
    std::ostringstream escaped;
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': escaped << "\\\""; break;
        case '\\': escaped << "\\\\"; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default:
            if (character < 0x20)
            {
                escaped << "\\u"
                        << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(character)
                        << std::dec << std::setfill(' ');
            }
            else
            {
                escaped << static_cast<char>(character);
            }
            break;
        }
    }
    return escaped.str();
}

std::string normalizedEvidencePayload(
    const EpgScraperMetadata& metadata)
{
    std::ostringstream payload;
    payload
        << "{\"schema\":1"
        << ",\"provider\":\"tvscraper\""
        << ",\"mediaType\":\"" << mediaTypeName(metadata.mediaType) << "\""
        << ",\"providerId\":" << metadata.providerId
        << ",\"title\":\"" << escapeJson(metadata.title) << "\""
        << ",\"originalTitle\":\"" << escapeJson(metadata.originalTitle) << "\""
        << ",\"episodeName\":\"" << escapeJson(metadata.episodeName) << "\""
        << ",\"seasonNumber\":" << metadata.seasonNumber
        << ",\"episodeNumber\":" << metadata.episodeNumber
        << ",\"imdbId\":\"" << escapeJson(metadata.imdbId) << "\""
        << ",\"people\":[";

    for (std::size_t index = 0; index < metadata.people.size(); ++index)
    {
        if (index > 0)
        {
            payload << ',';
        }

        const EpgScraperPerson& person = metadata.people.at(index);
        payload
            << "{\"name\":\"" << escapeJson(person.name) << "\""
            << ",\"normalizedName\":\""
            << escapeJson(PersonNameNormalizer::normalize(person.name)) << "\""
            << ",\"role\":\"" << roleName(domainRole(person.role)) << "\""
            << ",\"characterName\":\""
            << escapeJson(person.characterName) << "\""
            << ",\"imageAvailable\":"
            << (person.image.valid() ? "true" : "false")
            << ",\"imageIndex\":" << index
            << '}';
    }

    payload << "]}";
    return payload.str();
}

std::uint64_t fnv1a64(
    const std::string& value,
    std::uint64_t seed)
{
    std::uint64_t hash = seed;
    for (const unsigned char character : value)
    {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string stableHex(const std::string& value)
{
    const std::uint64_t first =
        fnv1a64(value, 14695981039346656037ULL);
    const std::uint64_t second =
        fnv1a64(value, 7809847782465536322ULL);

    std::ostringstream result;
    result << std::hex << std::setfill('0')
           << std::setw(16) << first
           << std::setw(16) << second;
    return result.str();
}

std::string targetId(
    const std::string& backendId,
    const VdrEvent& event)
{
    return "mdtgt_" + stableHex(
        "program-event\n" + backendId + "\n" +
        event.channelId + "\n" + event.id);
}

std::string sourceExternalId(const VdrEvent& event)
{
    return event.channelId + ":" + event.id;
}

int boundedLimit(int limit)
{
    if (limit <= 0)
    {
        return 50;
    }
    return std::min(limit, 100);
}

int boundedOffset(int offset)
{
    return std::max(0, std::min(offset, 100000));
}

bool beginTransaction(sqlite3* database)
{
    return sqlite3_exec(
        database,
        "BEGIN IMMEDIATE TRANSACTION;",
        nullptr,
        nullptr,
        nullptr) == SQLITE_OK;
}

bool finishTransaction(sqlite3* database, bool commit)
{
    return sqlite3_exec(
        database,
        commit ? "COMMIT;" : "ROLLBACK;",
        nullptr,
        nullptr,
        nullptr) == SQLITE_OK;
}

}

EpgPersonIndexRepository::EpgPersonIndexRepository(Database& database)
    : database_(database)
{
}

bool EpgPersonIndexRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool EpgPersonIndexRepository::ensureSchemaLocked() const
{
    MetadataPlatformSchemaInstaller installer(database_);
    if (!installer.ensureSchema())
    {
        return false;
    }

    return database_.execute(
        "CREATE TABLE IF NOT EXISTS suite_metadata_epg_person_index ("
        "backend_id TEXT NOT NULL,"
        "channel_id TEXT NOT NULL,"
        "event_id TEXT NOT NULL,"
        "original_name TEXT NOT NULL,"
        "normalized_name TEXT NOT NULL,"
        "role TEXT NOT NULL,"
        "character_name TEXT NOT NULL DEFAULT '',"
        "provider_person_id TEXT NOT NULL DEFAULT '',"
        "identity_kind TEXT NOT NULL DEFAULT 'name-only',"
        "confidence REAL NOT NULL DEFAULT 0.65,"
        "person_image_index INTEGER NOT NULL DEFAULT -1,"
        "metadata_evidence_id TEXT NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY ("
        "backend_id, channel_id, event_id, normalized_name, role, "
        "character_name, provider_person_id"
        "),"
        "CHECK (normalized_name <> ''),"
        "CHECK (role IN ("
        "'unknown','actor','director','writer','producer',"
        "'moderator','guest','composer','other'"
        ")),"
        "CHECK (identity_kind IN ('provider-id','name-only')),"
        "CHECK (confidence >= 0.0 AND confidence <= 1.0),"
        "FOREIGN KEY (metadata_evidence_id) "
        "REFERENCES suite_metadata_evidence(metadata_evidence_id) "
        "ON DELETE RESTRICT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_epg_person_name_time "
        "ON suite_metadata_epg_person_index ("
        "normalized_name, backend_id, channel_id, event_id"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_epg_person_provider_id "
        "ON suite_metadata_epg_person_index ("
        "provider_person_id, backend_id"
        ");"
        "INSERT OR IGNORE INTO suite_metadata_providers ("
        "provider_id, provider_kind, display_name, lifecycle_state, "
        "attribution_required"
        ") VALUES ('tvscraper','plugin','TVScraper','active',0);"
    );
}

bool EpgPersonIndexRepository::replaceEvidenceForEvent(
    const std::string& backendId,
    const VdrEvent& event,
    const EpgScraperMetadata& metadata)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    if (!ensureSchemaLocked() ||
        normalizedBackendId.empty() ||
        event.channelId.empty() ||
        event.id.empty() ||
        !metadata.valid())
    {
        return false;
    }

    const std::string payload = normalizedEvidencePayload(metadata);
    const std::string fingerprint = stableHex(payload);
    const std::string eventTargetId = targetId(normalizedBackendId, event);
    const std::string evidenceId = "mdevd_" + stableHex(
        eventTargetId + "\n" + fingerprint);

    sqlite3* database = database_.handle();
    if (!database || !beginTransaction(database))
    {
        return false;
    }

    bool ok = true;
    sqlite3_stmt* statement = nullptr;

    const char* targetSql =
        "INSERT OR IGNORE INTO suite_metadata_targets ("
        "metadata_target_id,target_type,lifecycle_state,revision"
        ") VALUES (?,'program-event','active',1);";
    ok = sqlite3_prepare_v2(
        database, targetSql, -1, &statement, nullptr) == SQLITE_OK &&
        bindText(statement, 1, eventTargetId) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    statement = nullptr;

    if (ok)
    {
        const char* scopeSql =
            "INSERT OR IGNORE INTO suite_metadata_provider_scopes ("
            "provider_id,scope_type,backend_id,enabled,priority,runtime_state"
            ") VALUES ('tvscraper','backend',?,1,100,'active');";
        ok = sqlite3_prepare_v2(
            database, scopeSql, -1, &statement, nullptr) == SQLITE_OK &&
            bindText(statement, 1, normalizedBackendId) &&
            sqlite3_step(statement) == SQLITE_DONE;
        sqlite3_finalize(statement);
        statement = nullptr;
    }

    if (ok)
    {
        const char* evidenceSql =
            "INSERT OR IGNORE INTO suite_metadata_evidence ("
            "metadata_evidence_id,metadata_target_id,provider_id,backend_id,"
            "source_entity_type,source_external_id,observed_at,language,"
            "provider_revision,payload_schema_version,normalized_payload,"
            "payload_fingerprint,confidence,evidence_state"
            ") VALUES (?,?, 'tvscraper', ?, 'program-event', ?, ?, '', '', "
            "1, ?, ?, 0.65, 'observed');";
        ok = sqlite3_prepare_v2(
            database, evidenceSql, -1, &statement, nullptr) == SQLITE_OK &&
            bindText(statement, 1, evidenceId) &&
            bindText(statement, 2, eventTargetId) &&
            bindText(statement, 3, normalizedBackendId) &&
            bindText(statement, 4, sourceExternalId(event)) &&
            bindText(statement, 5, std::to_string(epochSeconds())) &&
            bindText(statement, 6, payload) &&
            bindText(statement, 7, fingerprint) &&
            sqlite3_step(statement) == SQLITE_DONE;
        sqlite3_finalize(statement);
        statement = nullptr;
    }

    if (ok)
    {
        const char* deleteSql =
            "DELETE FROM suite_metadata_epg_person_index "
            "WHERE backend_id=? AND channel_id=? AND event_id=?;";
        ok = sqlite3_prepare_v2(
            database, deleteSql, -1, &statement, nullptr) == SQLITE_OK &&
            bindText(statement, 1, normalizedBackendId) &&
            bindText(statement, 2, event.channelId) &&
            bindText(statement, 3, event.id) &&
            sqlite3_step(statement) == SQLITE_DONE;
        sqlite3_finalize(statement);
        statement = nullptr;
    }

    const char* insertSql =
        "INSERT OR REPLACE INTO suite_metadata_epg_person_index ("
        "backend_id,channel_id,event_id,original_name,normalized_name,role,"
        "character_name,provider_person_id,identity_kind,confidence,"
        "person_image_index,metadata_evidence_id,updated_at"
        ") VALUES (?,?,?,?,?,?,?,'','name-only',0.65,?,?,CURRENT_TIMESTAMP);";

    for (std::size_t index = 0; ok && index < metadata.people.size(); ++index)
    {
        const EpgScraperPerson& scraperPerson = metadata.people.at(index);
        const std::string normalizedName =
            PersonNameNormalizer::normalize(scraperPerson.name);
        if (!scraperPerson.valid() || normalizedName.empty())
        {
            continue;
        }

        ok = sqlite3_prepare_v2(
            database, insertSql, -1, &statement, nullptr) == SQLITE_OK &&
            bindText(statement, 1, normalizedBackendId) &&
            bindText(statement, 2, event.channelId) &&
            bindText(statement, 3, event.id) &&
            bindText(statement, 4, scraperPerson.name) &&
            bindText(statement, 5, normalizedName) &&
            bindText(statement, 6, roleName(domainRole(scraperPerson.role))) &&
            bindText(statement, 7, scraperPerson.characterName) &&
            sqlite3_bind_int(
                statement,
                8,
                scraperPerson.image.valid()
                    ? static_cast<int>(index)
                    : -1) == SQLITE_OK &&
            bindText(statement, 9, evidenceId) &&
            sqlite3_step(statement) == SQLITE_DONE;
        sqlite3_finalize(statement);
        statement = nullptr;
    }

    return finishTransaction(database, ok) && ok;
}

std::vector<EpgPersonIndexMatch> EpgPersonIndexRepository::search(
    const EpgPersonIndexQuery& query) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<EpgPersonIndexMatch> matches;

    if (!ensureSchemaLocked() || query.normalizedName.empty())
    {
        return matches;
    }

    const bool allBackends =
        query.backendId.empty() || query.backendId == "all";
    const bool providerFilter = !query.providerPersonId.empty();
    const std::string fromTime = query.fromTime.empty()
        ? std::to_string(epochSeconds())
        : query.fromTime;

    std::ostringstream sql;
    sql
        << "SELECT p.backend_id,p.channel_id,p.event_id,p.original_name,"
        << "p.normalized_name,p.role,p.character_name,p.provider_person_id,"
        << "p.identity_kind,p.confidence,p.person_image_index,"
        << "p.metadata_evidence_id,e.title,e.subtitle,e.description,"
        << "e.start_time,e.end_time,e.duration_seconds,e.parental_rating "
        << "FROM suite_metadata_epg_person_index p "
        << "JOIN epg_events e ON e.backend_id=p.backend_id "
        << "AND e.channel_id=p.channel_id AND e.event_id=p.event_id "
        << "WHERE p.normalized_name=? AND e.end_time>=? ";
    if (!allBackends)
    {
        sql << "AND p.backend_id=? ";
    }
    if (providerFilter)
    {
        sql << "AND p.provider_person_id=? ";
    }
    sql << "ORDER BY e.start_time ASC,p.backend_id ASC "
        << "LIMIT ? OFFSET ?;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.str().c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
    {
        return matches;
    }

    int parameter = 1;
    bool bound =
        bindText(statement, parameter++, query.normalizedName) &&
        bindText(statement, parameter++, fromTime);
    if (!allBackends)
    {
        bound = bound && bindText(
            statement,
            parameter++,
            normalizeBackendId(query.backendId));
    }
    if (providerFilter)
    {
        bound = bound && bindText(
            statement,
            parameter++,
            query.providerPersonId);
    }
    bound = bound &&
        sqlite3_bind_int(statement, parameter++, boundedLimit(query.limit)) == SQLITE_OK &&
        sqlite3_bind_int(statement, parameter, boundedOffset(query.offset)) == SQLITE_OK;

    while (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        EpgPersonIndexMatch match;
        match.person.backendId = columnText(statement, 0);
        match.person.channelId = columnText(statement, 1);
        match.person.eventId = columnText(statement, 2);
        match.person.originalName = columnText(statement, 3);
        match.person.normalizedName = columnText(statement, 4);
        match.person.role = roleFromName(columnText(statement, 5));
        match.person.characterName = columnText(statement, 6);
        match.person.providerPersonId = columnText(statement, 7);
        match.person.identityKind = columnText(statement, 8);
        match.person.confidence = sqlite3_column_double(statement, 9);
        match.person.personImageIndex = sqlite3_column_int(statement, 10);
        match.person.metadataEvidenceId = columnText(statement, 11);

        match.event.channelId = match.person.channelId;
        match.event.id = match.person.eventId;
        match.event.title = columnText(statement, 12);
        match.event.subtitle = columnText(statement, 13);
        match.event.description = columnText(statement, 14);
        match.event.startTime = columnText(statement, 15);
        match.event.endTime = columnText(statement, 16);
        match.event.durationSeconds = sqlite3_column_int(statement, 17);
        match.event.parentalRating = sqlite3_column_int(statement, 18);
        matches.push_back(std::move(match));
    }

    sqlite3_finalize(statement);
    return matches;
}

int EpgPersonIndexRepository::count(
    const EpgPersonIndexQuery& query) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || query.normalizedName.empty())
    {
        return 0;
    }

    const bool allBackends =
        query.backendId.empty() || query.backendId == "all";
    const bool providerFilter = !query.providerPersonId.empty();
    const std::string fromTime = query.fromTime.empty()
        ? std::to_string(epochSeconds())
        : query.fromTime;

    std::ostringstream sql;
    sql
        << "SELECT COUNT(*) FROM suite_metadata_epg_person_index p "
        << "JOIN epg_events e ON e.backend_id=p.backend_id "
        << "AND e.channel_id=p.channel_id AND e.event_id=p.event_id "
        << "WHERE p.normalized_name=? AND e.end_time>=? ";
    if (!allBackends) sql << "AND p.backend_id=? ";
    if (providerFilter) sql << "AND p.provider_person_id=? ";
    sql << ';';

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(), sql.str().c_str(), -1,
            &statement, nullptr) != SQLITE_OK)
    {
        return 0;
    }

    int parameter = 1;
    bool bound =
        bindText(statement, parameter++, query.normalizedName) &&
        bindText(statement, parameter++, fromTime);
    if (!allBackends)
    {
        bound = bound && bindText(
            statement, parameter++, normalizeBackendId(query.backendId));
    }
    if (providerFilter)
    {
        bound = bound && bindText(
            statement, parameter++, query.providerPersonId);
    }

    int total = 0;
    if (bound && sqlite3_step(statement) == SQLITE_ROW)
    {
        total = sqlite3_column_int(statement, 0);
    }
    sqlite3_finalize(statement);
    return total;
}

bool EpgPersonIndexRepository::deleteExpiredForBackend(
    const std::string& backendId,
    const std::string& beforeEndTime)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || beforeEndTime.empty())
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM suite_metadata_epg_person_index "
        "WHERE backend_id=? AND EXISTS ("
        "SELECT 1 FROM epg_events e WHERE e.backend_id="
        "suite_metadata_epg_person_index.backend_id AND e.channel_id="
        "suite_metadata_epg_person_index.channel_id AND e.event_id="
        "suite_metadata_epg_person_index.event_id AND e.end_time<?"
        ");";

    const bool prepared = sqlite3_prepare_v2(
        database_.handle(), sql, -1, &statement, nullptr) == SQLITE_OK;
    const bool ok = prepared &&
        bindText(statement, 1, normalizeBackendId(backendId)) &&
        bindText(statement, 2, beforeEndTime) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

std::string EpgPersonIndexRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}
