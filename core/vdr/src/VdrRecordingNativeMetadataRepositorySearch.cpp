#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

#include <algorithm>
#include <utility>

using namespace vdr_recording_native_repository_detail;

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

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const std::string name = folded(query.name);
    const std::string characterName = folded(query.characterName);
    const char* where =
        " FROM vdr_recording_native_person p "
        "JOIN vdr_recording_native_metadata m "
        "ON m.backend_id=p.backend_id AND m.recording_key=p.recording_key "
        "WHERE p.backend_id=? AND m.content_state='found' "
        "AND (?='' OR instr(p.name_folded, ?) > 0) "
        "AND (?='' OR p.normalized_name=?) "
        "AND (?='' OR instr(p.character_name_folded, ?) > 0) "
        "AND (?='' OR p.role=? ) ";

    const std::string countSql = std::string("SELECT COUNT(*)") + where + ";";
    Statement countStatement(database_.handle(), countSql.c_str());
    if (!countStatement.valid()) return result;
    int index = 1;
    const bool countBound = bindText(countStatement.get(), index++, normalizedBackendId) &&
        bindText(countStatement.get(), index++, name) && bindText(countStatement.get(), index++, name) &&
        bindText(countStatement.get(), index++, query.normalizedName) && bindText(countStatement.get(), index++, query.normalizedName) &&
        bindText(countStatement.get(), index++, characterName) && bindText(countStatement.get(), index++, characterName) &&
        bindText(countStatement.get(), index++, query.role) && bindText(countStatement.get(), index++, query.role);
    if (!countBound || sqlite3_step(countStatement.get()) != SQLITE_ROW) return result;
    result.totalCount = sqlite3_column_int(countStatement.get(), 0);

    std::string selectSql = std::string(
        "SELECT p.recording_key, p.ordinal, p.role, p.name, p.normalized_name, p.character_name, "
        "p.image_provider, p.image_path, p.image_width, p.image_height") + where +
        "ORDER BY p.name_folded ASC, p.recording_key ASC, p.ordinal ASC ";
    selectSql += result.limit > 0 ? "LIMIT ? OFFSET ?;" : "LIMIT -1 OFFSET ?;";
    Statement selectStatement(database_.handle(), selectSql.c_str());
    if (!selectStatement.valid()) return result;

    index = 1;
    bool selectBound = bindText(selectStatement.get(), index++, normalizedBackendId) &&
        bindText(selectStatement.get(), index++, name) && bindText(selectStatement.get(), index++, name) &&
        bindText(selectStatement.get(), index++, query.normalizedName) && bindText(selectStatement.get(), index++, query.normalizedName) &&
        bindText(selectStatement.get(), index++, characterName) && bindText(selectStatement.get(), index++, characterName) &&
        bindText(selectStatement.get(), index++, query.role) && bindText(selectStatement.get(), index++, query.role);
    if (result.limit > 0) selectBound = selectBound && sqlite3_bind_int(selectStatement.get(), index++, result.limit) == SQLITE_OK;
    selectBound = selectBound && sqlite3_bind_int(selectStatement.get(), index++, result.offset) == SQLITE_OK;
    if (!selectBound) return result;

    while (sqlite3_step(selectStatement.get()) == SQLITE_ROW)
    {
        VdrRecordingNativePersonIndexEntry entry;
        entry.backendId = normalizedBackendId;
        entry.recordingKey = columnText(selectStatement.get(), 0);
        entry.ordinal = sqlite3_column_int(selectStatement.get(), 1);
        entry.role = columnText(selectStatement.get(), 2);
        entry.name = columnText(selectStatement.get(), 3);
        entry.normalizedName = columnText(selectStatement.get(), 4);
        entry.characterName = columnText(selectStatement.get(), 5);
        entry.image.provider = columnText(selectStatement.get(), 6);
        entry.image.path = columnText(selectStatement.get(), 7);
        entry.image.width = sqlite3_column_int(selectStatement.get(), 8);
        entry.image.height = sqlite3_column_int(selectStatement.get(), 9);
        entry.image.available = !entry.image.path.empty();
        result.entries.push_back(std::move(entry));
    }
    return result;
}
