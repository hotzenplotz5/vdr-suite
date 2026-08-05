#include "MetadataRepository.h"

#include "CurlExternalArtworkHttpTransport.h"
#include "Database.h"
#include "TmdbRecordingMetadataCandidateProvider.h"
#include "TmdbRecordingMetadataCredentialResolver.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <utility>

namespace
{
constexpr std::size_t MaximumMaterializedProfiles = 12U;
constexpr int MaximumConsecutiveProfileFailures = 2;
const std::string ManualArtworkRoot =
    "/var/cache/vdr-suite/recording-metadata/posters/";

std::string normalizedBackendId(const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string{}
        : std::string(reinterpret_cast<const char*>(value));
}

ManualRecordingMetadataAssignment readManualAssignment(
    sqlite3_stmt* statement,
    int offset)
{
    ManualRecordingMetadataAssignment assignment;
    assignment.found = true;
    assignment.backendId = columnText(statement, offset + 0);
    assignment.resourceKey = columnText(statement, offset + 1);
    assignment.metadataTargetId = columnText(statement, offset + 2);
    assignment.metadataAssignmentId = columnText(statement, offset + 3);
    assignment.metadataEntityId = columnText(statement, offset + 4);
    assignment.providerId = columnText(statement, offset + 5);
    assignment.externalNamespace = columnText(statement, offset + 6);
    assignment.externalId = columnText(statement, offset + 7);
    assignment.mediaType = columnText(statement, offset + 8);
    assignment.title = columnText(statement, offset + 9);
    assignment.originalTitle = columnText(statement, offset + 10);
    assignment.overview = columnText(statement, offset + 11);
    assignment.releaseDate = columnText(statement, offset + 12);
    assignment.posterReference = columnText(statement, offset + 13);
    assignment.seasonNumber = sqlite3_column_int(statement, offset + 14);
    assignment.episodeNumber = sqlite3_column_int(statement, offset + 15);
    assignment.actorRef = columnText(statement, offset + 16);
    assignment.revision = sqlite3_column_int(statement, offset + 17);
    assignment.relationshipLocked =
        sqlite3_column_int(statement, offset + 18) != 0;
    assignment.castComplete =
        sqlite3_column_int(statement, offset + 19) != 0;
    return assignment;
}

ManualRecordingMetadataPerson readManualPerson(
    sqlite3_stmt* statement,
    int offset)
{
    ManualRecordingMetadataPerson person;
    person.metadataEntityId = columnText(statement, offset + 0);
    person.providerId = columnText(statement, offset + 1);
    person.externalNamespace = columnText(statement, offset + 2);
    person.externalId = columnText(statement, offset + 3);
    person.name = columnText(statement, offset + 4);
    person.normalizedName = columnText(statement, offset + 5);
    person.role = columnText(statement, offset + 6);
    person.characterName = columnText(statement, offset + 7);
    person.ordinal = sqlite3_column_int(statement, offset + 8);
    return person;
}

std::string environmentOrEmpty(const char* name)
{
    const char* value = std::getenv(name);
    return value == nullptr ? std::string{} : std::string(value);
}

std::string resolveResourceKey(
    Database& database,
    const std::string& backendId,
    const std::string& suppliedKey)
{
    if (suppliedKey.empty() || !database.tableExists("vdr_recording_cache"))
        return suppliedKey;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT cache_key FROM vdr_recording_cache "
        "WHERE backend_id=? AND (cache_key=? OR backend_native_id=?) "
        "ORDER BY CASE WHEN cache_key=? THEN 0 ELSE 1 END LIMIT 1;";
    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return suppliedKey;

    const std::string backend = normalizedBackendId(backendId);
    sqlite3_bind_text(statement, 1, backend.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, suppliedKey.c_str(), -1, SQLITE_TRANSIENT);

    std::string resolved = suppliedKey;
    if (sqlite3_step(statement) == SQLITE_ROW)
    {
        const unsigned char* value = sqlite3_column_text(statement, 0);
        if (value != nullptr) resolved = reinterpret_cast<const char*>(value);
    }
    sqlite3_finalize(statement);
    return resolved;
}

bool localProfilePath(const std::string& path)
{
    if (path.empty()) return false;
    const std::filesystem::path normalized =
        std::filesystem::path(path).lexically_normal();
    return normalized.is_absolute() &&
        normalized != normalized.root_path() &&
        normalized.string().compare(
            0,
            ManualArtworkRoot.size(),
            ManualArtworkRoot) == 0;
}

std::string personKey(
    const std::string& providerId,
    const std::string& externalNamespace,
    const std::string& externalId)
{
    return providerId + "\n" + externalNamespace + "\n" + externalId;
}

bool ensurePersonProfileSchema(Database& database)
{
    return database.execute(
        "CREATE TABLE IF NOT EXISTS suite_metadata_person_profiles("
        "provider_id TEXT NOT NULL,"
        "external_namespace TEXT NOT NULL,"
        "external_id TEXT NOT NULL,"
        "profile_reference TEXT NOT NULL DEFAULT '',"
        "local_path TEXT NOT NULL DEFAULT '',"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(provider_id,external_namespace,external_id),"
        "CHECK(external_namespace='person'));"
        "CREATE INDEX IF NOT EXISTS idx_suite_metadata_person_profiles_path "
        "ON suite_metadata_person_profiles(local_path,provider_id,external_id);"
        "INSERT OR IGNORE INTO suite_metadata_schema_versions(version,description) "
        "VALUES(9,'Suite-owned manual person portrait references and local cache paths');");
}

struct StoredProfile
{
    std::string reference;
    std::string path;
};

std::map<std::string, StoredProfile> loadStoredProfiles(Database& database)
{
    std::map<std::string, StoredProfile> profiles;
    if (!database.tableExists("suite_metadata_person_profiles")) return profiles;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT provider_id,external_namespace,external_id,"
        "profile_reference,local_path FROM suite_metadata_person_profiles;";
    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return profiles;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        StoredProfile profile;
        profile.reference = columnText(statement, 3);
        profile.path = columnText(statement, 4);
        if (!localProfilePath(profile.path)) profile.path.clear();
        profiles.emplace(
            personKey(
                columnText(statement, 0),
                columnText(statement, 1),
                columnText(statement, 2)),
            std::move(profile));
    }
    sqlite3_finalize(statement);
    return profiles;
}

void applyStoredProfiles(
    ManualRecordingMetadataAssignment& assignment,
    const std::map<std::string, StoredProfile>& profiles)
{
    for (ManualRecordingMetadataPerson& person : assignment.people)
    {
        const auto match = profiles.find(personKey(
            person.providerId,
            person.externalNamespace,
            person.externalId));
        if (match == profiles.end()) continue;
        person.profileReference = match->second.reference;
        person.profilePath = match->second.path;
    }
}

bool persistPersonProfiles(
    Database& database,
    const std::vector<ManualRecordingMetadataPerson>& people)
{
    if (!ensurePersonProfileSchema(database)) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO suite_metadata_person_profiles("
        "provider_id,external_namespace,external_id,profile_reference,local_path,updated_at) "
        "VALUES(?,?,?,?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(provider_id,external_namespace,external_id) DO UPDATE SET "
        "profile_reference=CASE WHEN excluded.profile_reference<>'' "
        "THEN excluded.profile_reference ELSE profile_reference END,"
        "local_path=CASE WHEN excluded.local_path<>'' "
        "THEN excluded.local_path ELSE local_path END,"
        "updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
        return false;

    bool ok = true;
    for (const ManualRecordingMetadataPerson& person : people)
    {
        if (person.profileReference.empty() && person.profilePath.empty()) continue;
        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
        ok = sqlite3_bind_text(
                statement, 1, person.providerId.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(
                statement, 2, person.externalNamespace.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(
                statement, 3, person.externalId.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(
                statement, 4, person.profileReference.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_bind_text(
                statement, 5, person.profilePath.c_str(), -1, SQLITE_TRANSIENT) == SQLITE_OK &&
            sqlite3_step(statement) == SQLITE_DONE;
        if (!ok) break;
    }
    sqlite3_finalize(statement);
    return ok;
}

std::string materializePosterIfConfigured(
    const ManualRecordingMetadataSelection& selection,
    const std::string& token)
{
    if (selection.posterReference.empty() ||
        selection.posterReference.compare(
            0,
            ManualArtworkRoot.size(),
            ManualArtworkRoot) == 0)
        return selection.posterReference;

    if (selection.providerId != "tmdb" || token.empty())
        return selection.posterReference;

    static CurlExternalArtworkHttpTransport transport(
        CurlExternalArtworkHttpTransportConfig{
            {"api.themoviedb.org", "image.tmdb.org"},
            "vdr-suite/manual-recording-metadata-poster"});
    TmdbRecordingMetadataCandidateProviderConfig config;
    config.readAccessToken = token;
    const std::string language = environmentOrEmpty(
        "VDR_SUITE_TMDB_LANGUAGE");
    if (!language.empty()) config.language = language;
    TmdbRecordingMetadataCandidateProvider provider(transport, config);
    return provider.materializePoster(
        selection.externalNamespace,
        selection.externalId,
        selection.posterReference);
}

void materializePersonProfilesIfConfigured(
    ManualRecordingMetadataSelection& selection,
    const std::string& token)
{
    if (selection.providerId != "tmdb" || token.empty() ||
        selection.people.empty())
        return;

    static CurlExternalArtworkHttpTransport transport(
        CurlExternalArtworkHttpTransportConfig{
            {"api.themoviedb.org", "image.tmdb.org"},
            "vdr-suite/manual-recording-person-profile"});
    TmdbRecordingMetadataCandidateProviderConfig config;
    config.readAccessToken = token;
    config.connectTimeoutMs = 750;
    config.totalTimeoutMs = 2000;
    config.maximumRetries = 0;
    config.maximumImageBytes = 2U * 1024U * 1024U;
    const std::string language = environmentOrEmpty(
        "VDR_SUITE_TMDB_LANGUAGE");
    if (!language.empty()) config.language = language;
    TmdbRecordingMetadataCandidateProvider provider(transport, config);

    std::size_t attempts = 0U;
    int consecutiveFailures = 0;
    for (ManualRecordingMetadataPerson& person : selection.people)
    {
        if (person.profileReference.empty()) continue;
        if (attempts >= MaximumMaterializedProfiles ||
            consecutiveFailures >= MaximumConsecutiveProfileFailures)
            break;
        ++attempts;
        person.profilePath = provider.materializePoster(
            person.externalNamespace,
            person.externalId,
            person.profileReference);
        if (person.profilePath.empty()) ++consecutiveFailures;
        else consecutiveFailures = 0;
    }
}
}

ManualRecordingMetadataAssignmentRepository&
MetadataRepository::manualRepository()
{
    std::lock_guard<std::mutex> lock(manualMetadataRepositoryMutex_);
    if (!manualMetadataRepository_)
    {
        manualMetadataRepository_ =
            std::make_unique<ManualRecordingMetadataAssignmentRepository>(database_);
    }
    return *manualMetadataRepository_;
}

bool MetadataRepository::assignManualRecordingMetadata(
    const ManualRecordingMetadataSelection& selection,
    ManualRecordingMetadataAssignment& assigned)
{
    ManualRecordingMetadataAssignmentRepository& repository = manualRepository();
    if (!repository.ensureSchema()) return false;
    ensurePersonProfileSchema(database_);

    ManualRecordingMetadataSelection resolved = selection;
    resolved.backendId = normalizedBackendId(selection.backendId);
    resolved.resourceKey = resolveResourceKey(
        database_, resolved.backendId, selection.resourceKey);
    const std::string token =
        TmdbRecordingMetadataCredentialResolver::resolveReadAccessToken(
            resolved.backendId);
    const std::string materializedPoster =
        materializePosterIfConfigured(resolved, token);
    if (!resolved.posterReference.empty() &&
        !token.empty() &&
        resolved.providerId == "tmdb" && materializedPoster.empty())
        return false;
    resolved.posterReference = materializedPoster;
    materializePersonProfilesIfConfigured(resolved, token);

    if (!repository.assign(resolved, assigned)) return false;
    persistPersonProfiles(database_, resolved.people);
    applyStoredProfiles(assigned, loadStoredProfiles(database_));
    return true;
}

bool MetadataRepository::withdrawManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey,
    const std::string& actorRef,
    int expectedRevision,
    ManualRecordingMetadataAssignment& withdrawn)
{
    const std::string backend = normalizedBackendId(backendId);
    const bool ok = manualRepository().withdraw(
        backend,
        resolveResourceKey(database_, backend, resourceKey),
        actorRef,
        expectedRevision,
        withdrawn);
    if (ok)
    {
        ensurePersonProfileSchema(database_);
        applyStoredProfiles(withdrawn, loadStoredProfiles(database_));
    }
    return ok;
}

ManualRecordingMetadataAssignment
MetadataRepository::getManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey)
{
    const std::string backend = normalizedBackendId(backendId);
    ManualRecordingMetadataAssignment result = manualRepository().findSelected(
        backend,
        resolveResourceKey(database_, backend, resourceKey));
    ensurePersonProfileSchema(database_);
    applyStoredProfiles(result, loadStoredProfiles(database_));
    return result;
}

std::map<std::string, ManualRecordingMetadataAssignment>
MetadataRepository::getManualRecordingMetadataForBackend(
    const std::string& backendId)
{
    std::map<std::string, ManualRecordingMetadataAssignment> result;
    const std::string backend = normalizedBackendId(backendId);
    ManualRecordingMetadataAssignmentRepository& repository =
        manualRepository();
    if (!repository.ensureSchema()) return result;
    ensurePersonProfileSchema(database_);

    const bool cacheAvailable = database_.tableExists("vdr_recording_cache");
    const std::string lookup = cacheAvailable
        ? "COALESCE(NULLIF(c.backend_native_id,''),v.resource_key)"
        : "v.resource_key";
    const std::string cacheJoin = cacheAvailable
        ? "LEFT JOIN vdr_recording_cache c "
          "ON c.backend_id=v.backend_id AND c.cache_key=v.resource_key "
        : "";
    const std::string sql =
        "SELECT " + lookup + ","
        "v.backend_id,v.resource_key,v.metadata_target_id,v.metadata_assignment_id,"
        "a.metadata_entity_id,v.provider_id,v.external_namespace,v.external_id,"
        "v.media_type,v.title,v.original_title,v.overview,v.release_date,"
        "v.poster_reference,v.season_number,v.episode_number,v.actor_ref,"
        "v.revision,a.relationship_locked,v.cast_complete,"
        "r.person_entity_id,p.provider_id,p.external_namespace,p.external_id,"
        "p.display_name,p.normalized_name,r.role,r.character_name,r.ordinal "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id " + cacheJoin +
        "LEFT JOIN suite_metadata_recording_person_relations r "
        "ON r.metadata_assignment_id=v.metadata_assignment_id "
        "LEFT JOIN suite_metadata_person_values p "
        "ON p.metadata_entity_id=r.person_entity_id "
        "WHERE v.backend_id=? AND a.assignment_state='selected' "
        "AND a.manual_assignment=1 "
        "ORDER BY v.metadata_assignment_id,r.ordinal,p.name_folded,p.external_id;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_text(
        statement,
        1,
        backend.c_str(),
        static_cast<int>(backend.size()),
        SQLITE_TRANSIENT);

    std::map<std::string, ManualRecordingMetadataAssignment> assignments;
    std::map<std::string, std::string> aliases;
    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string lookupKey = columnText(statement, 0);
        const ManualRecordingMetadataAssignment row =
            readManualAssignment(statement, 1);
        auto insertion = assignments.emplace(
            row.metadataAssignmentId,
            row);
        ManualRecordingMetadataAssignment& assignment = insertion.first->second;
        const ManualRecordingMetadataPerson person =
            readManualPerson(statement, 21);
        if (!person.metadataEntityId.empty())
            assignment.people.push_back(person);

        if (!lookupKey.empty()) aliases[lookupKey] = row.metadataAssignmentId;
        if (!row.resourceKey.empty())
            aliases[row.resourceKey] = row.metadataAssignmentId;
    }
    sqlite3_finalize(statement);

    const std::map<std::string, StoredProfile> profiles =
        loadStoredProfiles(database_);
    for (auto& assignment : assignments)
        applyStoredProfiles(assignment.second, profiles);

    for (const auto& alias : aliases)
    {
        const auto assignment = assignments.find(alias.second);
        if (assignment != assignments.end())
            result[alias.first] = assignment->second;
    }
    return result;
}
