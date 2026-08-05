#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

#include <algorithm>
#include <string>
#include <utility>

using namespace vdr_recording_native_repository_detail;

namespace
{
std::string effectivePeopleCte(bool manualAvailable)
{
    if (!manualAvailable)
    {
        return
            "WITH effective_people AS ("
            "SELECT m.backend_native_id,p.recording_key,p.ordinal,p.role,p.name,"
            "p.name_folded,p.normalized_name,p.character_name,p.character_name_folded,"
            "'tvscraper' AS source,'' AS provider_reference,"
            "p.image_provider,p.image_path,p.image_width,p.image_height "
            "FROM vdr_recording_native_person p "
            "JOIN vdr_recording_native_metadata m "
            "ON m.backend_id=p.backend_id AND m.recording_key=p.recording_key "
            "WHERE p.backend_id=?1 AND m.content_state='found') ";
    }

    return
        "WITH effective_people AS ("
        "SELECT c.backend_native_id,v.resource_key AS recording_key,r.ordinal,r.role,"
        "p.display_name AS name,p.name_folded,p.normalized_name,r.character_name,"
        "r.character_name_folded,'tmdb' AS source,"
        "p.provider_id||':'||p.external_namespace||':'||p.external_id AS provider_reference,"
        "'' AS image_provider,'' AS image_path,0 AS image_width,0 AS image_height "
        "FROM suite_metadata_recording_person_relations r "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=r.metadata_assignment_id "
        "JOIN suite_metadata_manual_assignment_values v "
        "ON v.metadata_assignment_id=a.metadata_assignment_id "
        "JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "JOIN vdr_recording_cache c "
        "ON c.backend_id=v.backend_id AND c.cache_key=v.resource_key "
        "WHERE v.backend_id=?1 AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 AND a.relationship_locked=1 "
        "UNION ALL "
        "SELECT m.backend_native_id,p.recording_key,p.ordinal,p.role,p.name,"
        "p.name_folded,p.normalized_name,p.character_name,p.character_name_folded,"
        "'tvscraper' AS source,'' AS provider_reference,"
        "p.image_provider,p.image_path,p.image_width,p.image_height "
        "FROM vdr_recording_native_person p "
        "JOIN vdr_recording_native_metadata m "
        "ON m.backend_id=p.backend_id AND m.recording_key=p.recording_key "
        "LEFT JOIN vdr_recording_cache c "
        "ON c.backend_id=m.backend_id AND c.backend_native_id=m.backend_native_id "
        "WHERE p.backend_id=?1 AND m.content_state='found' "
        "AND NOT EXISTS("
        "SELECT 1 FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "WHERE v.backend_id=p.backend_id AND v.resource_key=c.cache_key "
        "AND a.assignment_state='selected' AND a.manual_assignment=1 "
        "AND a.relationship_locked=1)) ";
}

std::string effectiveWhere()
{
    return
        "WHERE (?2='' OR instr(p.name_folded,?2)>0) "
        "AND (?3='' OR p.normalized_name=?3) "
        "AND (?4='' OR instr(p.character_name_folded,?4)>0) "
        "AND (?5='' OR p.role=?5) "
        "AND (?6='' OR p.source=?6) "
        "AND (?7='' OR p.provider_reference=?7) ";
}

bool bindQuery(
    sqlite3_stmt* statement,
    const std::string& backendId,
    const std::string& name,
    const VdrRecordingNativePersonSearchQuery& query,
    const std::string& characterName)
{
    return bindText(statement, 1, backendId) &&
        bindText(statement, 2, name) &&
        bindText(statement, 3, query.normalizedName) &&
        bindText(statement, 4, characterName) &&
        bindText(statement, 5, query.role) &&
        bindText(statement, 6, query.source) &&
        bindText(statement, 7, query.providerReference);
}
}

VdrRecordingNativePersonSearchResult
VdrRecordingNativeMetadataRepository::searchPeople(
    const std::string& backendId,
    const VdrRecordingNativePersonSearchQuery& query) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    VdrRecordingNativePersonSearchResult result;
    result.limit = std::max(0, query.limit);
    result.offset = std::max(0, query.offset);
    if (!ensureSchemaLocked()) return result;

    const bool manualAvailable =
        database_.tableExists("suite_metadata_manual_assignment_values") &&
        database_.tableExists("suite_metadata_recording_person_relations") &&
        database_.tableExists("suite_metadata_person_values") &&
        database_.tableExists("vdr_recording_cache");
    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const std::string name = folded(query.name);
    const std::string characterName = folded(query.characterName);
    const std::string cte = effectivePeopleCte(manualAvailable);
    const std::string where = effectiveWhere();

    const std::string countSql =
        cte + "SELECT COUNT(*) FROM effective_people p " + where + ";";
    Statement countStatement(database_.handle(), countSql.c_str());
    if (!countStatement.valid() ||
        !bindQuery(
            countStatement.get(),
            normalizedBackendId,
            name,
            query,
            characterName) ||
        sqlite3_step(countStatement.get()) != SQLITE_ROW)
        return result;
    result.totalCount = sqlite3_column_int(countStatement.get(), 0);

    std::string selectSql = cte +
        "SELECT p.backend_native_id,p.recording_key,p.ordinal,p.role,p.name,"
        "p.normalized_name,p.character_name,p.source,p.provider_reference,"
        "p.image_provider,p.image_path,p.image_width,p.image_height "
        "FROM effective_people p " + where +
        "ORDER BY p.name_folded ASC,p.recording_key ASC,p.ordinal ASC,"
        "p.provider_reference ASC ";
    selectSql += result.limit > 0 ? "LIMIT ?8 OFFSET ?9;" : "LIMIT -1 OFFSET ?9;";
    Statement selectStatement(database_.handle(), selectSql.c_str());
    if (!selectStatement.valid() ||
        !bindQuery(
            selectStatement.get(),
            normalizedBackendId,
            name,
            query,
            characterName))
        return result;

    bool bound = true;
    if (result.limit > 0)
        bound = sqlite3_bind_int(selectStatement.get(), 8, result.limit) == SQLITE_OK;
    bound = bound &&
        sqlite3_bind_int(selectStatement.get(), 9, result.offset) == SQLITE_OK;
    if (!bound) return result;

    while (sqlite3_step(selectStatement.get()) == SQLITE_ROW)
    {
        VdrRecordingNativePersonIndexEntry entry;
        entry.backendId = normalizedBackendId;
        entry.backendNativeId = columnText(selectStatement.get(), 0);
        entry.recordingKey = columnText(selectStatement.get(), 1);
        entry.ordinal = sqlite3_column_int(selectStatement.get(), 2);
        entry.role = columnText(selectStatement.get(), 3);
        entry.name = columnText(selectStatement.get(), 4);
        entry.normalizedName = columnText(selectStatement.get(), 5);
        entry.characterName = columnText(selectStatement.get(), 6);
        entry.source = columnText(selectStatement.get(), 7);
        entry.providerReference = columnText(selectStatement.get(), 8);
        entry.image.provider = columnText(selectStatement.get(), 9);
        entry.image.path = columnText(selectStatement.get(), 10);
        entry.image.width = sqlite3_column_int(selectStatement.get(), 11);
        entry.image.height = sqlite3_column_int(selectStatement.get(), 12);
        entry.image.available = !entry.image.path.empty();
        result.entries.push_back(std::move(entry));
    }
    return result;
}
