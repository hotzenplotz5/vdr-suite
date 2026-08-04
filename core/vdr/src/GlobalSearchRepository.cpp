#include "GlobalSearchRepository.h"

#include "Database.h"
#include "VdrRecordingMetadataCacheCodec.h"

#include <sqlite3.h>

#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace
{
std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

bool bindNamedText(
    sqlite3_stmt* statement,
    const char* name,
    const std::string& value)
{
    const int index = sqlite3_bind_parameter_index(statement, name);
    return index > 0 && sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindNamedInt(
    sqlite3_stmt* statement,
    const char* name,
    int value)
{
    const int index = sqlite3_bind_parameter_index(statement, name);
    return index > 0 && sqlite3_bind_int(statement, index, value) == SQLITE_OK;
}

bool bindNamedInt64(
    sqlite3_stmt* statement,
    const char* name,
    std::int64_t value)
{
    const int index = sqlite3_bind_parameter_index(statement, name);
    return index > 0 && sqlite3_bind_int64(statement, index, value) == SQLITE_OK;
}

void sqliteFold(
    sqlite3_context* context,
    int argumentCount,
    sqlite3_value** arguments)
{
    if (argumentCount != 1 || sqlite3_value_type(arguments[0]) == SQLITE_NULL)
    {
        sqlite3_result_text(context, "", -1, SQLITE_STATIC);
        return;
    }

    const unsigned char* raw = sqlite3_value_text(arguments[0]);
    const std::string folded = GlobalSearchRepository::foldText(
        raw == nullptr ? std::string() : reinterpret_cast<const char*>(raw));
    sqlite3_result_text(
        context,
        folded.c_str(),
        static_cast<int>(folded.size()),
        SQLITE_TRANSIENT);
}

void sqliteRecordingMetadataText(
    sqlite3_context* context,
    int argumentCount,
    sqlite3_value** arguments)
{
    if (argumentCount != 1 || sqlite3_value_type(arguments[0]) == SQLITE_NULL)
    {
        sqlite3_result_text(context, "", -1, SQLITE_STATIC);
        return;
    }

    const unsigned char* raw = sqlite3_value_text(arguments[0]);
    const VdrRecordingMetadata metadata = VdrRecordingMetadataCacheCodec::decode(
        raw == nullptr ? std::string() : reinterpret_cast<const char*>(raw));
    const std::string searchable = GlobalSearchRepository::foldText(
        metadata.native.eventTitle + "\n" +
        metadata.native.shortText + "\n" +
        metadata.provider.title + "\n" +
        metadata.provider.originalTitle + "\n" +
        metadata.provider.seriesTitle + "\n" +
        metadata.provider.episodeTitle);
    sqlite3_result_text(
        context,
        searchable.c_str(),
        static_cast<int>(searchable.size()),
        SQLITE_TRANSIENT);
}

bool prepare(sqlite3* database, const std::string& sql, sqlite3_stmt** statement)
{
    return sqlite3_prepare_v2(
        database,
        sql.c_str(),
        -1,
        statement,
        nullptr) == SQLITE_OK;
}

std::string automaticTitlePredicate()
{
    return "(instr(vdr_suite_fold(c.title),:q)>0 "
        "OR instr(vdr_suite_fold(COALESCE(m.title,'')),:q)>0 "
        "OR instr(vdr_suite_fold(COALESCE(m.original_title,'')),:q)>0 "
        "OR instr(vdr_suite_fold(COALESCE(m.episode_name,'')),:q)>0 "
        "OR instr(vdr_suite_recording_metadata_text(c.metadata_payload),:q)>0)";
}

std::string recordingBaseSql(bool manualAvailable)
{
    const std::string nativePeople =
        "native_person_ranked AS ("
        "SELECT m.backend_id,m.backend_native_id,p.name,p.role,"
        "ROW_NUMBER() OVER(PARTITION BY m.backend_id,m.backend_native_id "
        "ORDER BY p.name_folded,p.ordinal) AS person_rank "
        "FROM vdr_recording_native_person p "
        "JOIN vdr_recording_native_metadata m "
        "ON m.backend_id=p.backend_id AND m.recording_key=p.recording_key "
        "WHERE p.backend_id=:backend AND m.content_state='found' "
        "AND instr(p.name_folded,:q)>0)";

    if (!manualAvailable)
    {
        return
            " WITH " + nativePeople + ","
            "search_base AS (SELECT c.recording_id,c.backend_id,"
            "c.backend_native_id,c.title,c.path,c.start_time,c.duration_seconds,"
            "c.size_mb,c.metadata_payload,COALESCE(m.episode_name,'') AS episode_name,"
            "COALESCE(m.preferred_artwork_path,'') AS preferred_artwork_path,"
            "CASE WHEN " + automaticTitlePredicate() +
            " THEN 1 ELSE 0 END AS title_hit,"
            "COALESCE(np.name,'') AS matched_person,"
            "COALESCE(np.role,'') AS matched_role "
            "FROM vdr_recording_cache c "
            "LEFT JOIN vdr_recording_native_metadata m "
            "ON m.backend_id=c.backend_id "
            "AND m.backend_native_id=c.backend_native_id "
            "AND m.content_state='found' "
            "LEFT JOIN native_person_ranked np "
            "ON np.backend_id=c.backend_id "
            "AND np.backend_native_id=c.backend_native_id "
            "AND np.person_rank=1 WHERE c.backend_id=:backend) ";
    }

    const std::string manualTitlePredicate =
        "(instr(vdr_suite_fold(c.title),:q)>0 "
        "OR instr(vdr_suite_fold(COALESCE(am.title,'')),:q)>0 "
        "OR instr(vdr_suite_fold(COALESCE(am.original_title,'')),:q)>0)";

    return
        " WITH active_manual AS ("
        "SELECT v.backend_id,v.resource_key,v.metadata_assignment_id,v.media_type,"
        "v.title,v.original_title,v.poster_reference "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "WHERE v.backend_id=:backend AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 AND a.relationship_locked=1),"
        "manual_person_ranked AS ("
        "SELECT v.backend_id,v.resource_key,p.display_name AS name,r.role,"
        "ROW_NUMBER() OVER(PARTITION BY v.backend_id,v.resource_key "
        "ORDER BY p.name_folded,r.ordinal,p.external_id) AS person_rank "
        "FROM suite_metadata_recording_person_relations r "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=r.metadata_assignment_id "
        "JOIN suite_metadata_manual_assignment_values v "
        "ON v.metadata_assignment_id=a.metadata_assignment_id "
        "JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "WHERE v.backend_id=:backend AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 AND a.relationship_locked=1 "
        "AND instr(p.name_folded,:q)>0)," +
        nativePeople + ","
        "search_base AS (SELECT c.recording_id,c.backend_id,c.backend_native_id,"
        "CASE WHEN am.metadata_assignment_id IS NOT NULL "
        "THEN COALESCE(NULLIF(am.title,''),c.title) ELSE c.title END AS title,"
        "c.path,c.start_time,c.duration_seconds,c.size_mb,c.metadata_payload,"
        "CASE WHEN am.media_type='episode' THEN am.title "
        "ELSE COALESCE(m.episode_name,'') END AS episode_name,"
        "CASE WHEN COALESCE(am.poster_reference,'')<>'' THEN am.poster_reference "
        "ELSE COALESCE(m.preferred_artwork_path,'') END AS preferred_artwork_path,"
        "CASE WHEN am.metadata_assignment_id IS NOT NULL THEN CASE WHEN " +
        manualTitlePredicate + " THEN 1 ELSE 0 END "
        "ELSE CASE WHEN " + automaticTitlePredicate() +
        " THEN 1 ELSE 0 END END AS title_hit,"
        "CASE WHEN am.metadata_assignment_id IS NOT NULL "
        "THEN COALESCE(mp.name,'') ELSE COALESCE(np.name,'') END AS matched_person,"
        "CASE WHEN am.metadata_assignment_id IS NOT NULL "
        "THEN COALESCE(mp.role,'') ELSE COALESCE(np.role,'') END AS matched_role "
        "FROM vdr_recording_cache c "
        "LEFT JOIN vdr_recording_native_metadata m "
        "ON m.backend_id=c.backend_id "
        "AND m.backend_native_id=c.backend_native_id "
        "AND m.content_state='found' "
        "LEFT JOIN active_manual am "
        "ON am.backend_id=c.backend_id AND am.resource_key=c.cache_key "
        "LEFT JOIN manual_person_ranked mp "
        "ON mp.backend_id=c.backend_id AND mp.resource_key=c.cache_key "
        "AND mp.person_rank=1 "
        "LEFT JOIN native_person_ranked np "
        "ON np.backend_id=c.backend_id "
        "AND np.backend_native_id=c.backend_native_id "
        "AND np.person_rank=1 WHERE c.backend_id=:backend) ";
}

std::string epgCandidateSql()
{
    return
        " WITH candidate_hits AS ("
        "SELECT e.backend_id,e.channel_id,e.event_id,1 AS title_hit,"
        "'' AS matched_person,'' AS matched_role "
        "FROM epg_events e WHERE e.backend_id=:backend "
        "AND CAST(e.end_time AS INTEGER)>=:from_time "
        "AND CAST(e.start_time AS INTEGER)<:until_time "
        "AND (instr(vdr_suite_fold(e.title),:q)>0 "
        "OR instr(vdr_suite_fold(e.subtitle),:q)>0) "
        "UNION ALL "
        "SELECT e.backend_id,e.channel_id,e.event_id,0 AS title_hit,"
        "p.name AS matched_person,p.role AS matched_role "
        "FROM epg_scraper_metadata_people p JOIN epg_events e "
        "ON e.backend_id=p.backend_id AND e.channel_id=p.channel_id "
        "AND e.event_id=p.event_id "
        "WHERE p.backend_id=:backend AND instr(p.name_folded,:q)>0 "
        "AND CAST(e.end_time AS INTEGER)>=:from_time "
        "AND CAST(e.start_time AS INTEGER)<:until_time),"
        "ranked_hits AS (SELECT backend_id,channel_id,event_id,"
        "MAX(title_hit) AS title_hit,"
        "COALESCE(MIN(NULLIF(matched_person,'')),'') AS matched_person,"
        "COALESCE(MIN(NULLIF(matched_role,'')),'') AS matched_role "
        "FROM candidate_hits GROUP BY backend_id,channel_id,event_id) ";
}

void addPersonSummary(
    std::map<std::string, GlobalSearchPersonSummary>& people,
    const std::string& name,
    const std::string& role,
    bool recording)
{
    if (name.empty()) return;
    const std::string key = GlobalSearchRepository::foldText(name) + "\n" + role;
    auto& summary = people[key];
    summary.name = name;
    summary.role = role;
    if (recording) ++summary.recordingCount;
    else ++summary.epgCount;
}
}

GlobalSearchRepository::GlobalSearchRepository(Database& database)
    : database_(database)
{
    registerFoldFunction();
}

bool GlobalSearchRepository::registerFoldFunction() const
{
    if (database_.handle() == nullptr) return false;
    const bool foldRegistered = sqlite3_create_function_v2(
        database_.handle(),
        "vdr_suite_fold",
        1,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr,
        sqliteFold,
        nullptr,
        nullptr,
        nullptr) == SQLITE_OK;
    const bool metadataRegistered = sqlite3_create_function_v2(
        database_.handle(),
        "vdr_suite_recording_metadata_text",
        1,
        SQLITE_UTF8 | SQLITE_DETERMINISTIC,
        nullptr,
        sqliteRecordingMetadataText,
        nullptr,
        nullptr,
        nullptr) == SQLITE_OK;
    return foldRegistered && metadataRegistered;
}

bool GlobalSearchRepository::ensureSchema()
{
    if (!registerFoldFunction()) return false;
    if (!database_.execute(
            "CREATE TABLE IF NOT EXISTS epg_scraper_metadata_people("
            "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
            "ordinal INTEGER NOT NULL,role TEXT NOT NULL DEFAULT 'unknown',"
            "name TEXT NOT NULL,name_folded TEXT NOT NULL,"
            "character_name TEXT NOT NULL DEFAULT '',character_name_folded TEXT NOT NULL DEFAULT '',"
            "PRIMARY KEY(backend_id,channel_id,event_id,ordinal));"
            "CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_people_name "
            "ON epg_scraper_metadata_people(backend_id,name_folded,channel_id,event_id);"
            "CREATE INDEX IF NOT EXISTS idx_epg_scraper_metadata_people_event "
            "ON epg_scraper_metadata_people(backend_id,channel_id,event_id);"))
    {
        return false;
    }
    return backfillEpgPeople();
}

bool GlobalSearchRepository::backfillEpgPeople() const
{
    if (!database_.tableExists("epg_scraper_metadata_cache")) return true;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO epg_scraper_metadata_people("
        "backend_id,channel_id,event_id,ordinal,role,name,name_folded,character_name,character_name_folded) "
        "SELECT c.backend_id,c.channel_id,c.event_id,CAST(j.key AS INTEGER),"
        "COALESCE(json_extract(j.value,'$.role'),'unknown'),"
        "COALESCE(json_extract(j.value,'$.name'),''),"
        "vdr_suite_fold(COALESCE(json_extract(j.value,'$.name'),'')),"
        "COALESCE(json_extract(j.value,'$.characterName'),''),"
        "vdr_suite_fold(COALESCE(json_extract(j.value,'$.characterName'),'')) "
        "FROM epg_scraper_metadata_cache c,json_each(c.public_json,'$.people') j "
        "WHERE json_valid(c.public_json) AND COALESCE(json_extract(j.value,'$.name'),'')<>'';";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return true;
    }
    const bool ok = sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return ok;
}

bool GlobalSearchRepository::ready() const
{
    return database_.handle() != nullptr &&
        database_.tableExists("vdr_recording_cache") &&
        database_.tableExists("vdr_recording_native_metadata") &&
        database_.tableExists("vdr_recording_native_person") &&
        database_.tableExists("epg_events") &&
        database_.tableExists("epg_scraper_metadata_cache") &&
        database_.tableExists("epg_scraper_metadata_people") &&
        database_.tableExists("epg_event_artwork");
}

GlobalSearchResult GlobalSearchRepository::search(
    const std::string& backendId,
    const std::string& query,
    std::int64_t epgFrom,
    std::int64_t epgUntil,
    int limit,
    int offset) const
{
    GlobalSearchResult result;
    result.query = query;
    result.backendId = normalizeBackendId(backendId);
    result.epgFrom = epgFrom;
    result.epgUntil = epgUntil;
    result.limit = std::max(1, limit);
    result.offset = std::max(0, offset);

    if (!ready())
    {
        result.status = "unavailable";
        return result;
    }

    searchRecordings(result);
    searchEpg(result);

    std::map<std::string, GlobalSearchPersonSummary> summaries;
    for (const auto& recording : result.recordings)
    {
        addPersonSummary(
            summaries,
            recording.matchedPerson,
            recording.matchedRole,
            true);
    }
    for (const auto& event : result.epg)
    {
        addPersonSummary(
            summaries,
            event.matchedPerson,
            event.matchedRole,
            false);
    }
    for (const auto& entry : summaries) result.people.push_back(entry.second);
    return result;
}

void GlobalSearchRepository::searchRecordings(GlobalSearchResult& result) const
{
    const std::string foldedQuery = foldText(result.query);
    const bool manualAvailable =
        database_.tableExists("suite_metadata_manual_assignment_values") &&
        database_.tableExists("suite_metadata_recording_person_relations") &&
        database_.tableExists("suite_metadata_person_values") &&
        database_.tableExists("suite_metadata_assignments");
    const std::string base = recordingBaseSql(manualAvailable);

    sqlite3_stmt* count = nullptr;
    const std::string countSql = base +
        "SELECT COUNT(*) FROM search_base WHERE title_hit=1 OR matched_person<>'';";
    if (!prepare(database_.handle(), countSql, &count)) return;
    bindNamedText(count, ":backend", result.backendId);
    bindNamedText(count, ":q", foldedQuery);
    if (sqlite3_step(count) == SQLITE_ROW)
    {
        result.recordingTotal = sqlite3_column_int(count, 0);
    }
    sqlite3_finalize(count);

    const std::string selectSql = base +
        "SELECT recording_id,backend_id,backend_native_id,title,path,start_time,"
        "duration_seconds,size_mb,metadata_payload,episode_name,preferred_artwork_path,"
        "matched_person,matched_role,title_hit "
        "FROM search_base WHERE title_hit=1 OR matched_person<>'' "
        "ORDER BY CASE "
        "WHEN vdr_suite_fold(title)=:q THEN 0 "
        "WHEN vdr_suite_fold(title) LIKE :q||'%' THEN 1 "
        "WHEN title_hit=1 THEN 2 ELSE 3 END ASC,"
        "CAST(start_time AS INTEGER) DESC,vdr_suite_fold(title) ASC,backend_native_id ASC "
        "LIMIT :limit OFFSET :offset;";
    sqlite3_stmt* statement = nullptr;
    if (!prepare(database_.handle(), selectSql, &statement)) return;
    const bool bound =
        bindNamedText(statement, ":backend", result.backendId) &&
        bindNamedText(statement, ":q", foldedQuery) &&
        bindNamedInt(statement, ":limit", result.limit) &&
        bindNamedInt(statement, ":offset", result.offset);
    if (!bound)
    {
        sqlite3_finalize(statement);
        return;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        GlobalSearchRecording item;
        item.id = columnText(statement, 0);
        item.backendId = columnText(statement, 1);
        item.backendNativeId = columnText(statement, 2);
        item.title = columnText(statement, 3);
        item.path = columnText(statement, 4);
        item.startTime = columnText(statement, 5);
        item.durationSeconds = sqlite3_column_int(statement, 6);
        item.sizeMb = sqlite3_column_int64(statement, 7);
        const VdrRecordingMetadata metadata =
            VdrRecordingMetadataCacheCodec::decode(columnText(statement, 8));
        item.subtitle = metadata.provider.episodeTitle;
        if (item.subtitle.empty()) item.subtitle = metadata.native.shortText;
        if (item.subtitle.empty()) item.subtitle = columnText(statement, 9);
        item.artworkAvailable = !columnText(statement, 10).empty() ||
            !metadata.artwork.empty();
        item.matchedPerson = columnText(statement, 11);
        item.matchedRole = columnText(statement, 12);
        const bool titleHit = sqlite3_column_int(statement, 13) != 0;
        item.matchReason = titleHit
            ? (item.matchedPerson.empty() ? "title" : "title-and-person")
            : "person";
        result.recordings.push_back(std::move(item));
    }
    sqlite3_finalize(statement);
    result.recordingHasMore =
        result.offset + static_cast<int>(result.recordings.size()) <
        result.recordingTotal;
}

void GlobalSearchRepository::searchEpg(GlobalSearchResult& result) const
{
    const std::string foldedQuery = foldText(result.query);
    const bool hasChannelCache = database_.tableExists("vdr_channel_cache");
    const std::string channelName = hasChannelCache
        ? "COALESCE(ch.name,e.channel_id)"
        : "e.channel_id";
    const std::string channelJoin = hasChannelCache
        ? " LEFT JOIN vdr_channel_cache ch ON ch.backend_id=e.backend_id AND ch.channel_id=e.channel_id "
        : " ";

    const std::string sql = epgCandidateSql() +
        "SELECT e.backend_id,e.channel_id," + channelName + " AS channel_name,"
        "e.event_id,e.title,e.subtitle,e.description,e.start_time,e.end_time,"
        "e.duration_seconds,"
        "CASE WHEN EXISTS(SELECT 1 FROM epg_event_artwork a "
        "WHERE a.backend_id=e.backend_id AND a.channel_id=e.channel_id "
        "AND a.event_id=e.event_id) THEN 1 ELSE 0 END AS artwork_available,"
        "h.matched_person,h.matched_role,h.title_hit,COUNT(*) OVER() AS total_count "
        "FROM ranked_hits h JOIN epg_events e "
        "ON e.backend_id=h.backend_id AND e.channel_id=h.channel_id "
        "AND e.event_id=h.event_id" + channelJoin +
        "ORDER BY CASE WHEN vdr_suite_fold(e.title)=:q THEN 0 "
        "WHEN vdr_suite_fold(e.title) LIKE :q||'%' THEN 1 "
        "WHEN h.title_hit=1 THEN 2 ELSE 3 END ASC,"
        "CAST(e.start_time AS INTEGER) ASC,vdr_suite_fold(e.title) ASC,"
        "e.channel_id ASC,e.event_id ASC LIMIT :limit OFFSET :offset;";

    sqlite3_stmt* statement = nullptr;
    if (!prepare(database_.handle(), sql, &statement)) return;
    const bool bound =
        bindNamedText(statement, ":backend", result.backendId) &&
        bindNamedText(statement, ":q", foldedQuery) &&
        bindNamedInt64(statement, ":from_time", result.epgFrom) &&
        bindNamedInt64(statement, ":until_time", result.epgUntil) &&
        bindNamedInt(statement, ":limit", result.limit) &&
        bindNamedInt(statement, ":offset", result.offset);
    if (!bound)
    {
        sqlite3_finalize(statement);
        return;
    }

    bool totalRead = false;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        if (!totalRead)
        {
            result.epgTotal = sqlite3_column_int(statement, 14);
            totalRead = true;
        }
        GlobalSearchEpgEvent item;
        item.backendId = columnText(statement, 0);
        item.channelId = columnText(statement, 1);
        item.channelName = columnText(statement, 2);
        item.eventId = columnText(statement, 3);
        item.title = columnText(statement, 4);
        item.subtitle = columnText(statement, 5);
        item.description = columnText(statement, 6);
        item.startTime = columnText(statement, 7);
        item.endTime = columnText(statement, 8);
        item.durationSeconds = sqlite3_column_int(statement, 9);
        item.artworkAvailable = sqlite3_column_int(statement, 10) != 0;
        item.matchedPerson = columnText(statement, 11);
        item.matchedRole = columnText(statement, 12);
        const bool titleHit = sqlite3_column_int(statement, 13) != 0;
        item.matchReason = titleHit
            ? (item.matchedPerson.empty() ? "title" : "title-and-person")
            : "person";
        result.epg.push_back(std::move(item));
    }
    sqlite3_finalize(statement);
    result.epgHasMore = result.offset + static_cast<int>(result.epg.size()) < result.epgTotal;
}

std::string GlobalSearchRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string GlobalSearchRepository::foldText(const std::string& value)
{
    std::string folded;
    folded.reserve(value.size());
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        const unsigned char byte = static_cast<unsigned char>(value[index]);
        if (byte >= 'A' && byte <= 'Z')
        {
            folded.push_back(static_cast<char>(byte - 'A' + 'a'));
            continue;
        }
        if (byte == 0xc3 && index + 1 < value.size())
        {
            const unsigned char next = static_cast<unsigned char>(value[index + 1]);
            if (next == 0x84 || next == 0xa4) { folded += "ae"; ++index; continue; }
            if (next == 0x96 || next == 0xb6) { folded += "oe"; ++index; continue; }
            if (next == 0x9c || next == 0xbc) { folded += "ue"; ++index; continue; }
            if (next == 0x9f) { folded += "ss"; ++index; continue; }
            if ((next >= 0x80 && next <= 0x96) || (next >= 0x98 && next <= 0x9e))
            {
                folded.push_back(static_cast<char>(0xc3));
                folded.push_back(static_cast<char>(next + 0x20));
                ++index;
                continue;
            }
        }
        folded.push_back(static_cast<char>(byte));
    }
    return folded;
}
