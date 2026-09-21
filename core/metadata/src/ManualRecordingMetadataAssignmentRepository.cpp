#include "ManualRecordingMetadataAssignmentRepository.h"

#include "Database.h"
#include "GenreIndexRepository.h"
#include "MetadataIdentity.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        static_cast<int>(value.size()),
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool stepDone(sqlite3_stmt* statement)
{
    return sqlite3_step(statement) == SQLITE_DONE;
}

std::string columnText(sqlite3_stmt* statement, int index)
{
    const unsigned char* value = sqlite3_column_text(statement, index);
    return value == nullptr ? std::string{} :
        std::string(reinterpret_cast<const char*>(value));
}

bool tableHasColumn(
    sqlite3* database,
    const std::string& tableName,
    const std::string& columnName)
{
    sqlite3_stmt* statement = nullptr;
    const std::string sql = "PRAGMA table_info(" + tableName + ");";
    if (sqlite3_prepare_v2(
            database,
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
        return false;

    bool found = false;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        if (columnText(statement, 1) == columnName)
        {
            found = true;
            break;
        }
    }
    sqlite3_finalize(statement);
    return found;
}

bool boundedText(
    const std::string& value,
    std::size_t maximum,
    bool allowEmpty = true)
{
    if ((!allowEmpty && value.empty()) || value.size() > maximum) return false;
    return std::none_of(value.begin(), value.end(), [](unsigned char character) {
        return character < 0x20U && character != '\t';
    });
}

bool digits(const std::string& value)
{
    return !value.empty() && value.size() <= 16U &&
        std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return character >= '0' && character <= '9';
        });
}

bool validMediaType(const std::string& value)
{
    return value == "movie" || value == "series" || value == "episode";
}

bool validPosterReference(const std::string& value)
{
    if (value.empty()) return true;
    return boundedText(value, 1024U, false) &&
        value.front() == '/' &&
        value.find("://") == std::string::npos &&
        value.find("..") == std::string::npos;
}

std::string folded(const std::string& value)
{
    std::string result = value;
    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character) {
            return static_cast<char>(std::tolower(character));
        });
    return result;
}

std::string normalizedPersonName(const std::string& value)
{
    std::string result;
    bool separatorPending = false;
    for (const unsigned char character : value)
    {
        if ((character >= 'a' && character <= 'z') ||
            (character >= '0' && character <= '9'))
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character));
        }
        else if (character >= 'A' && character <= 'Z')
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character - 'A' + 'a'));
        }
        else if (character >= 0x80U)
        {
            if (separatorPending && !result.empty()) result.push_back('-');
            separatorPending = false;
            result.push_back(static_cast<char>(character));
        }
        else
        {
            separatorPending = !result.empty();
        }
    }
    return result;
}

bool validPerson(const ManualRecordingMetadataPerson& person)
{
    return MetadataProviderId::isValidValue(person.providerId) &&
        person.providerId == "tmdb" &&
        person.externalNamespace == "person" &&
        digits(person.externalId) &&
        boundedText(person.name, 512U, false) &&
        boundedText(person.normalizedName, 512U) &&
        person.role == "actor" &&
        boundedText(person.characterName, 512U) &&
        person.ordinal >= 0 && person.ordinal < 100000;
}

bool validSelection(const ManualRecordingMetadataSelection& selection)
{
    if (!boundedText(selection.backendId, 128U) ||
        !boundedText(selection.resourceKey, 4096U, false) ||
        !MetadataProviderId::isValidValue(selection.providerId) ||
        selection.providerId == "manual" ||
        !boundedText(selection.externalNamespace, 64U, false) ||
        !boundedText(selection.externalId, 256U, false) ||
        !validMediaType(selection.mediaType) ||
        !boundedText(selection.title, 512U, false) ||
        !boundedText(selection.originalTitle, 512U) ||
        !boundedText(selection.overview, 16384U) ||
        !boundedText(selection.releaseDate, 64U) ||
        !validPosterReference(selection.posterReference) ||
        selection.seasonNumber < 0 || selection.seasonNumber > 10000 ||
        selection.episodeNumber < 0 || selection.episodeNumber > 100000 ||
        !boundedText(selection.actorRef, 256U, false) ||
        selection.expectedRevision < 0 ||
        selection.people.size() > 128U ||
        (!selection.castComplete && !selection.people.empty()))
        return false;

    return std::all_of(
        selection.people.begin(),
        selection.people.end(),
        validPerson);
}

std::string jsonEscape(const std::string& value)
{
    static const char Hex[] = "0123456789abcdef";
    std::string output;
    output.reserve(value.size() + 16U);
    for (const unsigned char character : value)
    {
        switch (character)
        {
        case '"': output += "\\\""; break;
        case '\\': output += "\\\\"; break;
        case '\b': output += "\\b"; break;
        case '\f': output += "\\f"; break;
        case '\n': output += "\\n"; break;
        case '\r': output += "\\r"; break;
        case '\t': output += "\\t"; break;
        default:
            if (character < 0x20U)
            {
                output += "\\u00";
                output.push_back(Hex[(character >> 4U) & 0x0fU]);
                output.push_back(Hex[character & 0x0fU]);
            }
            else
            {
                output.push_back(static_cast<char>(character));
            }
        }
    }
    return output;
}

std::vector<ManualRecordingMetadataPerson> normalizedPeople(
    const ManualRecordingMetadataSelection& selection)
{
    std::map<std::string, ManualRecordingMetadataPerson> unique;
    for (ManualRecordingMetadataPerson person : selection.people)
    {
        if (person.normalizedName.empty())
            person.normalizedName = normalizedPersonName(person.name);
        const std::string key = person.providerId + "\n" +
            person.externalNamespace + "\n" + person.externalId;
        const auto existing = unique.find(key);
        if (existing == unique.end() || person.ordinal < existing->second.ordinal)
            unique[key] = std::move(person);
    }

    std::vector<ManualRecordingMetadataPerson> people;
    people.reserve(unique.size());
    for (auto& entry : unique) people.push_back(std::move(entry.second));
    std::sort(
        people.begin(),
        people.end(),
        [](const ManualRecordingMetadataPerson& left,
           const ManualRecordingMetadataPerson& right) {
            if (left.ordinal != right.ordinal) return left.ordinal < right.ordinal;
            if (left.name != right.name) return left.name < right.name;
            return left.externalId < right.externalId;
        });
    return people;
}

std::string normalizedPayload(
    const ManualRecordingMetadataSelection& selection,
    const std::vector<ManualRecordingMetadataPerson>& people)
{
    std::string payload = std::string("{") +
        "\"provider\":\"" + jsonEscape(selection.providerId) + "\"," +
        "\"namespace\":\"" + jsonEscape(selection.externalNamespace) + "\"," +
        "\"externalId\":\"" + jsonEscape(selection.externalId) + "\"," +
        "\"mediaType\":\"" + jsonEscape(selection.mediaType) + "\"," +
        "\"title\":\"" + jsonEscape(selection.title) + "\"," +
        "\"originalTitle\":\"" + jsonEscape(selection.originalTitle) + "\"," +
        "\"overview\":\"" + jsonEscape(selection.overview) + "\"," +
        "\"releaseDate\":\"" + jsonEscape(selection.releaseDate) + "\"," +
        "\"posterReference\":\"" + jsonEscape(selection.posterReference) + "\"," +
        "\"seasonNumber\":" + std::to_string(selection.seasonNumber) + "," +
        "\"episodeNumber\":" + std::to_string(selection.episodeNumber) + "," +
        "\"castComplete\":" + (selection.castComplete ? "true" : "false") +
        ",\"cast\":[";
    for (std::size_t index = 0; index < people.size(); ++index)
    {
        if (index > 0U) payload += ',';
        const ManualRecordingMetadataPerson& person = people[index];
        payload += std::string("{") +
            "\"provider\":\"" + jsonEscape(person.providerId) + "\"," +
            "\"namespace\":\"" + jsonEscape(person.externalNamespace) + "\"," +
            "\"externalId\":\"" + jsonEscape(person.externalId) + "\"," +
            "\"name\":\"" + jsonEscape(person.name) + "\"," +
            "\"role\":\"" + jsonEscape(person.role) + "\"," +
            "\"character\":\"" + jsonEscape(person.characterName) + "\"," +
            "\"order\":" + std::to_string(person.ordinal) + "}";
    }
    payload += "]}";
    return payload;
}

bool insertProvider(
    sqlite3* database,
    const std::string& providerId,
    const std::string& providerKind,
    const std::string& displayName)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO suite_metadata_providers"
        "(provider_id,provider_kind,display_name,lifecycle_state,attribution_required) "
        "VALUES(?,?,?,'active',?);";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool bound =
        bindText(statement, 1, providerId) &&
        bindText(statement, 2, providerKind) &&
        bindText(statement, 3, displayName) &&
        sqlite3_bind_int(statement, 4, providerKind == "external-catalog" ? 1 : 0)
            == SQLITE_OK;
    const bool ok = bound && stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool insertProviderScope(
    sqlite3* database,
    const std::string& providerId,
    const std::string& scopeType,
    const std::string& backendId)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT OR IGNORE INTO suite_metadata_provider_scopes"
        "(provider_id,scope_type,backend_id,enabled,priority,runtime_state) "
        "VALUES(?,?,?,1,100,'active');";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool ok =
        bindText(statement, 1, providerId) &&
        bindText(statement, 2, scopeType) &&
        bindText(statement, 3, backendId) &&
        stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool upsertTarget(
    sqlite3* database,
    const std::string& targetId,
    const std::string& backendId,
    const std::string& resourceKey)
{
    sqlite3_stmt* target = nullptr;
    const char* targetSql =
        "INSERT INTO suite_metadata_targets"
        "(metadata_target_id,target_type,lifecycle_state,revision,updated_at) "
        "VALUES(?,'recording','active',1,CURRENT_TIMESTAMP) "
        "ON CONFLICT(metadata_target_id) DO UPDATE SET "
        "lifecycle_state='active',updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, targetSql, -1, &target, nullptr) != SQLITE_OK)
        return false;
    const bool targetOk = bindText(target, 1, targetId) && stepDone(target);
    sqlite3_finalize(target);
    if (!targetOk) return false;

    sqlite3_stmt* binding = nullptr;
    const char* bindingSql =
        "INSERT INTO suite_metadata_target_bindings"
        "(metadata_target_id,target_type,backend_id,resource_key,native_id,lifecycle_state,updated_at) "
        "VALUES(?,'recording',?,?,?,'active',CURRENT_TIMESTAMP) "
        "ON CONFLICT(target_type,backend_id,resource_key) DO UPDATE SET "
        "metadata_target_id=excluded.metadata_target_id,native_id=excluded.native_id,"
        "lifecycle_state='active',updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, bindingSql, -1, &binding, nullptr) != SQLITE_OK)
        return false;
    const bool bindingOk =
        bindText(binding, 1, targetId) &&
        bindText(binding, 2, backendId) &&
        bindText(binding, 3, resourceKey) &&
        bindText(binding, 4, resourceKey) &&
        stepDone(binding);
    sqlite3_finalize(binding);
    return bindingOk;
}

struct CurrentAssignment
{
    std::string assignmentId;
    int revision = 0;
};

CurrentAssignment currentAssignment(
    sqlite3* database,
    const std::string& targetId)
{
    CurrentAssignment current;
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT metadata_assignment_id,revision FROM suite_metadata_assignments "
        "WHERE metadata_target_id=? AND assignment_state='selected' LIMIT 1;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return current;
    if (bindText(statement, 1, targetId) && sqlite3_step(statement) == SQLITE_ROW)
    {
        current.assignmentId = columnText(statement, 0);
        current.revision = sqlite3_column_int(statement, 1);
    }
    sqlite3_finalize(statement);
    return current;
}

bool updateAssignmentState(
    sqlite3* database,
    const std::string& assignmentId,
    const std::string& state)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "UPDATE suite_metadata_assignments SET assignment_state=?,"
        "revision=revision+1,updated_at=CURRENT_TIMESTAMP "
        "WHERE metadata_assignment_id=? AND assignment_state='selected';";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool ok =
        bindText(statement, 1, state) &&
        bindText(statement, 2, assignmentId) &&
        stepDone(statement) &&
        sqlite3_changes(database) == 1;
    sqlite3_finalize(statement);
    return ok;
}

std::string findPersonEntity(
    sqlite3* database,
    const ManualRecordingMetadataPerson& person)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT e.metadata_entity_id "
        "FROM suite_metadata_entity_external_ids x "
        "JOIN suite_metadata_entities e "
        "ON e.metadata_entity_id=x.metadata_entity_id "
        "WHERE x.provider_id=? AND x.external_namespace=? AND x.external_id=? "
        "AND x.binding_state='active' AND e.media_type='person' "
        "AND e.lifecycle_state='active' ORDER BY e.metadata_entity_id LIMIT 1;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return {};
    std::string entityId;
    if (bindText(statement, 1, person.providerId) &&
        bindText(statement, 2, person.externalNamespace) &&
        bindText(statement, 3, person.externalId) &&
        sqlite3_step(statement) == SQLITE_ROW)
        entityId = columnText(statement, 0);
    sqlite3_finalize(statement);
    return entityId;
}

bool upsertPersonValue(
    sqlite3* database,
    const std::string& entityId,
    const ManualRecordingMetadataPerson& person)
{
    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO suite_metadata_person_values("
        "metadata_entity_id,provider_id,external_namespace,external_id,"
        "display_name,name_folded,normalized_name,updated_at) "
        "VALUES(?,?,?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(metadata_entity_id) DO UPDATE SET "
        "display_name=excluded.display_name,name_folded=excluded.name_folded,"
        "normalized_name=excluded.normalized_name,updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;
    const bool ok =
        bindText(statement, 1, entityId) &&
        bindText(statement, 2, person.providerId) &&
        bindText(statement, 3, person.externalNamespace) &&
        bindText(statement, 4, person.externalId) &&
        bindText(statement, 5, person.name) &&
        bindText(statement, 6, folded(person.name)) &&
        bindText(statement, 7, person.normalizedName) &&
        stepDone(statement);
    sqlite3_finalize(statement);
    return ok;
}

bool persistPeople(
    sqlite3* database,
    const std::vector<ManualRecordingMetadataPerson>& people,
    const std::string& assignmentId,
    const std::string& targetId,
    const std::string& evidenceId)
{
    for (const ManualRecordingMetadataPerson& person : people)
    {
        std::string personEntityId = findPersonEntity(database, person);
        if (personEntityId.empty())
        {
            const MetadataEntityId generated = MetadataEntityId::generate();
            if (!generated.isValid()) return false;
            personEntityId = generated.value();

            sqlite3_stmt* entity = nullptr;
            const char* entitySql =
                "INSERT INTO suite_metadata_entities"
                "(metadata_entity_id,media_type,lifecycle_state,revision) "
                "VALUES(?,'person','active',1);";
            const bool entityOk =
                sqlite3_prepare_v2(database, entitySql, -1, &entity, nullptr) == SQLITE_OK &&
                bindText(entity, 1, personEntityId) &&
                stepDone(entity);
            sqlite3_finalize(entity);
            if (!entityOk) return false;
        }

        sqlite3_stmt* external = nullptr;
        const char* externalSql =
            "INSERT OR IGNORE INTO suite_metadata_entity_external_ids"
            "(metadata_entity_id,provider_id,external_namespace,external_id,"
            "metadata_evidence_id,binding_state) VALUES(?,?,?,?,?,'active');";
        const bool externalOk =
            sqlite3_prepare_v2(database, externalSql, -1, &external, nullptr) == SQLITE_OK &&
            bindText(external, 1, personEntityId) &&
            bindText(external, 2, person.providerId) &&
            bindText(external, 3, person.externalNamespace) &&
            bindText(external, 4, person.externalId) &&
            bindText(external, 5, evidenceId) &&
            stepDone(external);
        sqlite3_finalize(external);
        if (!externalOk || !upsertPersonValue(database, personEntityId, person))
            return false;

        sqlite3_stmt* relation = nullptr;
        const char* relationSql =
            "INSERT INTO suite_metadata_recording_person_relations("
            "metadata_assignment_id,metadata_target_id,person_entity_id,"
            "metadata_evidence_id,role,character_name,character_name_folded,ordinal) "
            "VALUES(?,?,?,?,?,?,?,?);";
        const bool relationOk =
            sqlite3_prepare_v2(database, relationSql, -1, &relation, nullptr) == SQLITE_OK &&
            bindText(relation, 1, assignmentId) &&
            bindText(relation, 2, targetId) &&
            bindText(relation, 3, personEntityId) &&
            bindText(relation, 4, evidenceId) &&
            bindText(relation, 5, person.role) &&
            bindText(relation, 6, person.characterName) &&
            bindText(relation, 7, folded(person.characterName)) &&
            sqlite3_bind_int(relation, 8, person.ordinal) == SQLITE_OK &&
            stepDone(relation);
        sqlite3_finalize(relation);
        if (!relationOk) return false;
    }
    return true;
}

void loadPeople(
    sqlite3* database,
    ManualRecordingMetadataAssignment& assignment)
{
    assignment.people.clear();
    if (assignment.metadataAssignmentId.empty()) return;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT r.person_entity_id,p.provider_id,p.external_namespace,p.external_id,"
        "p.display_name,p.normalized_name,r.role,r.character_name,r.ordinal "
        "FROM suite_metadata_recording_person_relations r "
        "JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "WHERE r.metadata_assignment_id=? "
        "ORDER BY r.ordinal,p.name_folded,p.external_id;";
    if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK)
        return;
    if (!bindText(statement, 1, assignment.metadataAssignmentId))
    {
        sqlite3_finalize(statement);
        return;
    }

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        ManualRecordingMetadataPerson person;
        person.metadataEntityId = columnText(statement, 0);
        person.providerId = columnText(statement, 1);
        person.externalNamespace = columnText(statement, 2);
        person.externalId = columnText(statement, 3);
        person.name = columnText(statement, 4);
        person.normalizedName = columnText(statement, 5);
        person.role = columnText(statement, 6);
        person.characterName = columnText(statement, 7);
        person.ordinal = sqlite3_column_int(statement, 8);
        assignment.people.push_back(std::move(person));
    }
    sqlite3_finalize(statement);
}
}

ManualRecordingMetadataAssignmentRepository::
ManualRecordingMetadataAssignmentRepository(Database& database)
    : database_(database)
{
}

bool ManualRecordingMetadataAssignmentRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool ManualRecordingMetadataAssignmentRepository::ensureSchemaLocked() const
{
    if (schemaReady_) return true;

    GenreIndexRepository foundation(database_);
    if (!foundation.ensureSchema()) return false;

    const char* schema = R"sql(
INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description)
VALUES(7,'Manual recording metadata assignment values and withdrawal history');
CREATE TABLE IF NOT EXISTS suite_metadata_manual_assignment_values(
    metadata_assignment_id TEXT PRIMARY KEY,
    metadata_target_id TEXT NOT NULL,
    backend_id TEXT NOT NULL,
    resource_key TEXT NOT NULL,
    provider_id TEXT NOT NULL,
    external_namespace TEXT NOT NULL,
    external_id TEXT NOT NULL,
    media_type TEXT NOT NULL,
    title TEXT NOT NULL,
    original_title TEXT NOT NULL DEFAULT '',
    overview TEXT NOT NULL DEFAULT '',
    release_date TEXT NOT NULL DEFAULT '',
    poster_reference TEXT NOT NULL DEFAULT '',
    season_number INTEGER NOT NULL DEFAULT 0,
    episode_number INTEGER NOT NULL DEFAULT 0,
    actor_ref TEXT NOT NULL,
    revision INTEGER NOT NULL,
    cast_complete INTEGER NOT NULL DEFAULT 0,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK(media_type IN('movie','series','episode')),
    CHECK(season_number>=0),
    CHECK(episode_number>=0),
    CHECK(revision>0),
    CHECK(cast_complete IN(0,1)),
    FOREIGN KEY(metadata_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE CASCADE,
    FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE,
    FOREIGN KEY(provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_manual_values_target
ON suite_metadata_manual_assignment_values(metadata_target_id,revision);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_manual_values_external
ON suite_metadata_manual_assignment_values(provider_id,external_namespace,external_id);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_manual_values_backend_resource
ON suite_metadata_manual_assignment_values(backend_id,resource_key,metadata_assignment_id);
CREATE TABLE IF NOT EXISTS suite_metadata_manual_assignment_withdrawals(
    metadata_assignment_id TEXT PRIMARY KEY,
    metadata_target_id TEXT NOT NULL,
    withdrawn_by_ref TEXT NOT NULL,
    prior_revision INTEGER NOT NULL,
    withdrawn_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK(prior_revision>0),
    FOREIGN KEY(metadata_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE CASCADE,
    FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE
);
CREATE TABLE IF NOT EXISTS suite_metadata_person_values(
    metadata_entity_id TEXT PRIMARY KEY,
    provider_id TEXT NOT NULL,
    external_namespace TEXT NOT NULL,
    external_id TEXT NOT NULL,
    display_name TEXT NOT NULL,
    name_folded TEXT NOT NULL,
    normalized_name TEXT NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(provider_id,external_namespace,external_id),
    FOREIGN KEY(metadata_entity_id) REFERENCES suite_metadata_entities(metadata_entity_id) ON DELETE RESTRICT,
    FOREIGN KEY(provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_person_values_name
ON suite_metadata_person_values(provider_id,name_folded,normalized_name,metadata_entity_id);
CREATE TABLE IF NOT EXISTS suite_metadata_recording_person_relations(
    metadata_assignment_id TEXT NOT NULL,
    metadata_target_id TEXT NOT NULL,
    person_entity_id TEXT NOT NULL,
    metadata_evidence_id TEXT NOT NULL,
    role TEXT NOT NULL,
    character_name TEXT NOT NULL DEFAULT '',
    character_name_folded TEXT NOT NULL DEFAULT '',
    ordinal INTEGER NOT NULL,
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY(metadata_assignment_id,ordinal,person_entity_id),
    CHECK(role IN('actor')),
    CHECK(ordinal>=0),
    FOREIGN KEY(metadata_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE CASCADE,
    FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE,
    FOREIGN KEY(person_entity_id) REFERENCES suite_metadata_entities(metadata_entity_id) ON DELETE RESTRICT,
    FOREIGN KEY(metadata_evidence_id) REFERENCES suite_metadata_evidence(metadata_evidence_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_recording_people_assignment
ON suite_metadata_recording_person_relations(metadata_assignment_id,ordinal,person_entity_id);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_recording_people_person
ON suite_metadata_recording_person_relations(person_entity_id,metadata_assignment_id,ordinal);
INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description)
VALUES(8,'Manual recording cast entities, external identities and assignment-scoped relations');
)sql";

    if (!database_.execute(schema)) return false;
    if (!tableHasColumn(
            database_.handle(),
            "suite_metadata_manual_assignment_values",
            "cast_complete") &&
        !database_.execute(
            "ALTER TABLE suite_metadata_manual_assignment_values "
            "ADD COLUMN cast_complete INTEGER NOT NULL DEFAULT 0 "
            "CHECK(cast_complete IN(0,1));"))
        return false;

    schemaReady_ = true;
    return true;
}

bool ManualRecordingMetadataAssignmentRepository::assign(
    const ManualRecordingMetadataSelection& rawSelection,
    ManualRecordingMetadataAssignment& assigned)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    assigned = {};
    if (!ensureSchemaLocked() || !validSelection(rawSelection)) return false;

    ManualRecordingMetadataSelection selection = rawSelection;
    if (selection.backendId.empty()) selection.backendId = "default";
    const std::vector<ManualRecordingMetadataPerson> people =
        normalizedPeople(selection);
    if (people.size() > 128U) return false;

    const std::string targetId = GenreIndexRepository::stableTargetId(
        "recording", selection.backendId, selection.resourceKey);

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;

    sqlite3* database = database_.handle();
    const CurrentAssignment current = currentAssignment(database, targetId);
    if (selection.expectedRevision > 0 &&
        current.revision != selection.expectedRevision)
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    const int nextRevision = std::max(1, current.revision + 1);
    const MetadataEntityId entityId = MetadataEntityId::generate();
    const MetadataAssignmentId assignmentId = MetadataAssignmentId::generate();
    const MetadataEvidenceId evidenceId = MetadataEvidenceId::generate();
    if (!entityId.isValid() || !assignmentId.isValid() || !evidenceId.isValid() ||
        !insertProvider(database, "manual", "manual", "Manual assignment") ||
        !insertProvider(database, selection.providerId, "external-catalog", selection.providerId) ||
        !insertProviderScope(database, "manual", "global", "") ||
        !insertProviderScope(database, selection.providerId, "backend", selection.backendId) ||
        !upsertTarget(database, targetId, selection.backendId, selection.resourceKey))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    if (!current.assignmentId.empty() &&
        !updateAssignmentState(database, current.assignmentId, "superseded"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    sqlite3_stmt* entity = nullptr;
    const char* entitySql =
        "INSERT INTO suite_metadata_entities"
        "(metadata_entity_id,media_type,lifecycle_state,revision) "
        "VALUES(? ,? ,'active',1);";
    bool ok = sqlite3_prepare_v2(database, entitySql, -1, &entity, nullptr) == SQLITE_OK &&
        bindText(entity, 1, entityId.value()) &&
        bindText(entity, 2, selection.mediaType) &&
        stepDone(entity);
    sqlite3_finalize(entity);

    const std::string sourceExternalId =
        selection.providerId + ":" + selection.externalNamespace + ":" + selection.externalId;
    sqlite3_stmt* evidence = nullptr;
    const char* evidenceSql =
        "INSERT INTO suite_metadata_evidence"
        "(metadata_evidence_id,metadata_target_id,provider_id,backend_id,"
        "source_entity_type,source_external_id,observed_at,payload_schema_version,"
        "normalized_payload,confidence,evidence_state) "
        "VALUES(?,?, 'manual',?,?,?,strftime('%Y-%m-%dT%H:%M:%fZ','now'),2,?,1.0,'observed');";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, evidenceSql, -1, &evidence, nullptr) == SQLITE_OK &&
            bindText(evidence, 1, evidenceId.value()) &&
            bindText(evidence, 2, targetId) &&
            bindText(evidence, 3, selection.backendId) &&
            bindText(evidence, 4, selection.mediaType) &&
            bindText(evidence, 5, sourceExternalId) &&
            bindText(evidence, 6, normalizedPayload(selection, people)) &&
            stepDone(evidence);
    }
    sqlite3_finalize(evidence);

    sqlite3_stmt* assignment = nullptr;
    const char* assignmentSql =
        "INSERT INTO suite_metadata_assignments"
        "(metadata_assignment_id,metadata_target_id,metadata_entity_id,assignment_state,"
        "confidence,manual_assignment,relationship_locked,supersedes_assignment_id,"
        "created_by_ref,revision) VALUES(?,?,?,'selected',1.0,1,1,?,?,?);";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, assignmentSql, -1, &assignment, nullptr) == SQLITE_OK &&
            bindText(assignment, 1, assignmentId.value()) &&
            bindText(assignment, 2, targetId) &&
            bindText(assignment, 3, entityId.value()) &&
            (current.assignmentId.empty()
                ? sqlite3_bind_null(assignment, 4) == SQLITE_OK
                : bindText(assignment, 4, current.assignmentId)) &&
            bindText(assignment, 5, selection.actorRef) &&
            sqlite3_bind_int(assignment, 6, nextRevision) == SQLITE_OK &&
            stepDone(assignment);
    }
    sqlite3_finalize(assignment);

    sqlite3_stmt* link = nullptr;
    const char* linkSql =
        "INSERT INTO suite_metadata_assignment_evidence"
        "(metadata_assignment_id,metadata_evidence_id,evidence_role) "
        "VALUES(?,?,'manual-override');";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, linkSql, -1, &link, nullptr) == SQLITE_OK &&
            bindText(link, 1, assignmentId.value()) &&
            bindText(link, 2, evidenceId.value()) &&
            stepDone(link);
    }
    sqlite3_finalize(link);

    sqlite3_stmt* external = nullptr;
    const char* externalSql =
        "INSERT INTO suite_metadata_entity_external_ids"
        "(metadata_entity_id,provider_id,external_namespace,external_id,"
        "metadata_evidence_id,binding_state) VALUES(?,?,?,?,?,'active');";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, externalSql, -1, &external, nullptr) == SQLITE_OK &&
            bindText(external, 1, entityId.value()) &&
            bindText(external, 2, selection.providerId) &&
            bindText(external, 3, selection.externalNamespace) &&
            bindText(external, 4, selection.externalId) &&
            bindText(external, 5, evidenceId.value()) &&
            stepDone(external);
    }
    sqlite3_finalize(external);

    sqlite3_stmt* values = nullptr;
    const char* valuesSql =
        "INSERT INTO suite_metadata_manual_assignment_values"
        "(metadata_assignment_id,metadata_target_id,backend_id,resource_key,provider_id,"
        "external_namespace,external_id,media_type,title,original_title,overview,"
        "release_date,poster_reference,season_number,episode_number,actor_ref,revision,cast_complete) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, valuesSql, -1, &values, nullptr) == SQLITE_OK &&
            bindText(values, 1, assignmentId.value()) &&
            bindText(values, 2, targetId) &&
            bindText(values, 3, selection.backendId) &&
            bindText(values, 4, selection.resourceKey) &&
            bindText(values, 5, selection.providerId) &&
            bindText(values, 6, selection.externalNamespace) &&
            bindText(values, 7, selection.externalId) &&
            bindText(values, 8, selection.mediaType) &&
            bindText(values, 9, selection.title) &&
            bindText(values, 10, selection.originalTitle) &&
            bindText(values, 11, selection.overview) &&
            bindText(values, 12, selection.releaseDate) &&
            bindText(values, 13, selection.posterReference) &&
            sqlite3_bind_int(values, 14, selection.seasonNumber) == SQLITE_OK &&
            sqlite3_bind_int(values, 15, selection.episodeNumber) == SQLITE_OK &&
            bindText(values, 16, selection.actorRef) &&
            sqlite3_bind_int(values, 17, nextRevision) == SQLITE_OK &&
            sqlite3_bind_int(values, 18, selection.castComplete ? 1 : 0) == SQLITE_OK &&
            stepDone(values);
    }
    sqlite3_finalize(values);

    if (ok)
    {
        ok = persistPeople(
            database,
            people,
            assignmentId.value(),
            targetId,
            evidenceId.value());
    }

    if (!ok || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    assigned = findSelected(selection.backendId, selection.resourceKey);
    return assigned.found;
}

bool ManualRecordingMetadataAssignmentRepository::withdraw(
    const std::string& rawBackendId,
    const std::string& resourceKey,
    const std::string& actorRef,
    int expectedRevision,
    ManualRecordingMetadataAssignment& withdrawn)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    withdrawn = {};
    const std::string backendId = rawBackendId.empty() ? "default" : rawBackendId;
    if (!ensureSchemaLocked() ||
        !boundedText(backendId, 128U) ||
        !boundedText(resourceKey, 4096U, false) ||
        !boundedText(actorRef, 256U, false) ||
        expectedRevision <= 0)
        return false;

    const ManualRecordingMetadataAssignment current =
        findSelected(backendId, resourceKey);
    if (!current.found || current.revision != expectedRevision) return false;

    auto transactionLease = database_.acquireTransactionLease();
    if (!database_.execute("BEGIN IMMEDIATE TRANSACTION;")) return false;
    sqlite3* database = database_.handle();
    bool ok = updateAssignmentState(
        database, current.metadataAssignmentId, "withdrawn");

    sqlite3_stmt* history = nullptr;
    const char* historySql =
        "INSERT INTO suite_metadata_manual_assignment_withdrawals"
        "(metadata_assignment_id,metadata_target_id,withdrawn_by_ref,prior_revision) "
        "VALUES(?,?,?,?);";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, historySql, -1, &history, nullptr) == SQLITE_OK &&
            bindText(history, 1, current.metadataAssignmentId) &&
            bindText(history, 2, current.metadataTargetId) &&
            bindText(history, 3, actorRef) &&
            sqlite3_bind_int(history, 4, current.revision) == SQLITE_OK &&
            stepDone(history);
    }
    sqlite3_finalize(history);

    if (!ok || !database_.execute("COMMIT;"))
    {
        database_.execute("ROLLBACK;");
        return false;
    }

    withdrawn = current;
    withdrawn.relationshipLocked = false;
    return true;
}

ManualRecordingMetadataAssignment
ManualRecordingMetadataAssignmentRepository::findSelected(
    const std::string& rawBackendId,
    const std::string& resourceKey) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ManualRecordingMetadataAssignment result;
    const std::string backendId = rawBackendId.empty() ? "default" : rawBackendId;
    if (!ensureSchemaLocked() ||
        !boundedText(backendId, 128U) ||
        !boundedText(resourceKey, 4096U, false))
        return result;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT v.backend_id,v.resource_key,v.metadata_target_id,v.metadata_assignment_id,"
        "a.metadata_entity_id,v.provider_id,v.external_namespace,v.external_id,v.media_type,"
        "v.title,v.original_title,v.overview,v.release_date,v.poster_reference,"
        "v.season_number,v.episode_number,v.actor_ref,v.revision,a.relationship_locked,"
        "v.cast_complete "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "WHERE v.backend_id=? AND v.resource_key=? "
        "AND a.assignment_state='selected' AND a.manual_assignment=1 LIMIT 1;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return result;
    if (bindText(statement, 1, backendId) &&
        bindText(statement, 2, resourceKey) &&
        sqlite3_step(statement) == SQLITE_ROW)
    {
        result.found = true;
        result.backendId = columnText(statement, 0);
        result.resourceKey = columnText(statement, 1);
        result.metadataTargetId = columnText(statement, 2);
        result.metadataAssignmentId = columnText(statement, 3);
        result.metadataEntityId = columnText(statement, 4);
        result.providerId = columnText(statement, 5);
        result.externalNamespace = columnText(statement, 6);
        result.externalId = columnText(statement, 7);
        result.mediaType = columnText(statement, 8);
        result.title = columnText(statement, 9);
        result.originalTitle = columnText(statement, 10);
        result.overview = columnText(statement, 11);
        result.releaseDate = columnText(statement, 12);
        result.posterReference = columnText(statement, 13);
        result.seasonNumber = sqlite3_column_int(statement, 14);
        result.episodeNumber = sqlite3_column_int(statement, 15);
        result.actorRef = columnText(statement, 16);
        result.revision = sqlite3_column_int(statement, 17);
        result.relationshipLocked = sqlite3_column_int(statement, 18) != 0;
        result.castComplete = sqlite3_column_int(statement, 19) != 0;
    }
    sqlite3_finalize(statement);
    if (result.found) loadPeople(database_.handle(), result);
    return result;
}
