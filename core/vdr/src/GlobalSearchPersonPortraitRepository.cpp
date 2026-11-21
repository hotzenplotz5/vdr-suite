#include "GlobalSearchPersonPortraitRepository.h"

#include "Database.h"

#include <sqlite3.h>
#include <string>
#include <utility>
#include <vector>

namespace
{
std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string{}
        : std::string(reinterpret_cast<const char*>(value));
}
}

GlobalSearchPersonPortraitRepository::GlobalSearchPersonPortraitRepository(
    Database& database)
    : database_(database)
{
}

std::vector<GlobalSearchPersonPortrait>
GlobalSearchPersonPortraitRepository::findForBackend(
    const std::string& backendId) const
{
    std::vector<GlobalSearchPersonPortrait> portraits;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT v.metadata_assignment_id,p.display_name,r.role,"
        "COALESCE(NULLIF(c.backend_native_id,''),v.resource_key),"
        "v.revision "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "JOIN suite_metadata_recording_person_relations r "
        "ON r.metadata_assignment_id=v.metadata_assignment_id "
        "JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "JOIN suite_metadata_person_profiles profile "
        "ON profile.provider_id=p.provider_id "
        "AND profile.external_namespace=p.external_namespace "
        "AND profile.external_id=p.external_id "
        "LEFT JOIN vdr_recording_cache c "
        "ON c.backend_id=v.backend_id AND c.cache_key=v.resource_key "
        "WHERE v.backend_id=? "
        "AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 "
        "AND a.relationship_locked=1 "
        "AND profile.local_path<>'' "
        "ORDER BY v.revision DESC,v.metadata_assignment_id,"
        "r.ordinal,p.name_folded,p.external_id;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return portraits;

    sqlite3_bind_text(
        statement,
        1,
        backendId.c_str(),
        static_cast<int>(backendId.size()),
        SQLITE_TRANSIENT);

    std::string assignmentId;
    int personIndex = 0;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string rowAssignmentId = columnText(statement, 0);
        if (rowAssignmentId != assignmentId)
        {
            assignmentId = rowAssignmentId;
            personIndex = 0;
        }

        GlobalSearchPersonPortrait portrait;
        portrait.name = columnText(statement, 1);
        portrait.role = columnText(statement, 2);
        portrait.backendNativeId = columnText(statement, 3);
        portrait.index = personIndex++;
        portrait.assignmentRevision = sqlite3_column_int(statement, 4);
        if (!portrait.name.empty() && !portrait.backendNativeId.empty() &&
            portrait.assignmentRevision > 0)
            portraits.push_back(std::move(portrait));
    }
    sqlite3_finalize(statement);
    return portraits;
}