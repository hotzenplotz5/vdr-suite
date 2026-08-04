#include "ManualRecordingMetadataAssignmentRepository.h"

#include "Database.h"
#include "GenreIndexRepository.h"
#include "MetadataIdentity.h"

#include <sqlite3.h>

#include <algorithm>
#include <cctype>
#include <string>

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

bool validSelection(const ManualRecordingMetadataSelection& selection)
{
    return boundedText(selection.backendId, 128U) &&
        boundedText(selection.resourceKey, 4096U, false) &&
        MetadataProviderId::isValidValue(selection.providerId) &&
        selection.providerId != "manual" &&
        boundedText(selection.externalNamespace, 64U, false) &&
        boundedText(selection.externalId, 256U, false) &&
        validMediaType(selection.mediaType) &&
        boundedText(selection.title, 512U, false) &&
        boundedText(selection.originalTitle, 512U) &&
        boundedText(selection.overview, 16384U) &&
        boundedText(selection.releaseDate, 64U) &&
        validPosterReference(selection.posterReference) &&
        selection.seasonNumber >= 0 && selection.seasonNumber <= 10000 &&
        selection.episodeNumber >= 0 && selection.episodeNumber <= 100000 &&
        boundedText(selection.actorRef, 256U, false) &&
        selection.expectedRevision >= 0;
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

std::string normalizedPayload(
    const ManualRecordingMetadataSelection& selection)
{
    return std::string("{") +
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
        "\"episodeNumber\":" + std::to_string(selection.episodeNumber) +
        "}";
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
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    CHECK(media_type IN('movie','series','episode')),
    CHECK(season_number>=0),
    CHECK(episode_number>=0),
    CHECK(revision>0),
    FOREIGN KEY(metadata_assignment_id) REFERENCES suite_metadata_assignments(metadata_assignment_id) ON DELETE CASCADE,
    FOREIGN KEY(metadata_target_id) REFERENCES suite_metadata_targets(metadata_target_id) ON DELETE CASCADE,
    FOREIGN KEY(provider_id) REFERENCES suite_metadata_providers(provider_id) ON DELETE RESTRICT
);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_manual_values_target
ON suite_metadata_manual_assignment_values(metadata_target_id,revision);
CREATE INDEX IF NOT EXISTS idx_suite_metadata_manual_values_external
ON suite_metadata_manual_assignment_values(provider_id,external_namespace,external_id);
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
)sql";

    if (!database_.execute(schema)) return false;
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
        "VALUES(?,?, 'manual',?,?,?,strftime('%Y-%m-%dT%H:%M:%fZ','now'),1,?,1.0,'observed');";
    if (ok)
    {
        ok = sqlite3_prepare_v2(database, evidenceSql, -1, &evidence, nullptr) == SQLITE_OK &&
            bindText(evidence, 1, evidenceId.value()) &&
            bindText(evidence, 2, targetId) &&
            bindText(evidence, 3, selection.backendId) &&
            bindText(evidence, 4, selection.mediaType) &&
            bindText(evidence, 5, sourceExternalId) &&
            bindText(evidence, 6, normalizedPayload(selection)) &&
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
        "release_date,poster_reference,season_number,episode_number,actor_ref,revision) "
        "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
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
            stepDone(values);
    }
    sqlite3_finalize(values);

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
        "v.season_number,v.episode_number,v.actor_ref,v.revision,a.relationship_locked "
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
    }
    sqlite3_finalize(statement);
    return result;
}
