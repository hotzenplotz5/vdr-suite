#include "MetadataRepository.h"

#include "CurlExternalArtworkHttpTransport.h"
#include "Database.h"
#include "TmdbRecordingMetadataCandidateProvider.h"
#include "TmdbRecordingMetadataCredentialResolver.h"

#include <cstdlib>
#include <map>
#include <memory>
#include <sqlite3.h>

namespace
{
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
    return assignment;
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

std::string materializePosterIfConfigured(
    const ManualRecordingMetadataSelection& selection,
    const std::string& token)
{
    if (selection.posterReference.empty() ||
        selection.posterReference.compare(
            0,
            std::string("/var/cache/vdr-suite/recording-metadata/posters/").size(),
            "/var/cache/vdr-suite/recording-metadata/posters/") == 0)
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
    return manualRepository().assign(resolved, assigned);
}

bool MetadataRepository::withdrawManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey,
    const std::string& actorRef,
    int expectedRevision,
    ManualRecordingMetadataAssignment& withdrawn)
{
    const std::string backend = normalizedBackendId(backendId);
    return manualRepository().withdraw(
        backend,
        resolveResourceKey(database_, backend, resourceKey),
        actorRef,
        expectedRevision,
        withdrawn);
}

ManualRecordingMetadataAssignment
MetadataRepository::getManualRecordingMetadata(
    const std::string& backendId,
    const std::string& resourceKey)
{
    const std::string backend = normalizedBackendId(backendId);
    return manualRepository().findSelected(
        backend,
        resolveResourceKey(database_, backend, resourceKey));
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

    const bool cacheAvailable = database_.tableExists("vdr_recording_cache");
    const char* sqlWithCache =
        "SELECT COALESCE(NULLIF(c.backend_native_id,''),v.resource_key),"
        "v.backend_id,v.resource_key,v.metadata_target_id,v.metadata_assignment_id,"
        "a.metadata_entity_id,v.provider_id,v.external_namespace,v.external_id,"
        "v.media_type,v.title,v.original_title,v.overview,v.release_date,"
        "v.poster_reference,v.season_number,v.episode_number,v.actor_ref,"
        "v.revision,a.relationship_locked "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "LEFT JOIN vdr_recording_cache c "
        "ON c.backend_id=v.backend_id AND c.cache_key=v.resource_key "
        "WHERE v.backend_id=? AND a.assignment_state='selected' "
        "AND a.manual_assignment=1;";
    const char* sqlWithoutCache =
        "SELECT v.resource_key,"
        "v.backend_id,v.resource_key,v.metadata_target_id,v.metadata_assignment_id,"
        "a.metadata_entity_id,v.provider_id,v.external_namespace,v.external_id,"
        "v.media_type,v.title,v.original_title,v.overview,v.release_date,"
        "v.poster_reference,v.season_number,v.episode_number,v.actor_ref,"
        "v.revision,a.relationship_locked "
        "FROM suite_metadata_manual_assignment_values v "
        "JOIN suite_metadata_assignments a "
        "ON a.metadata_assignment_id=v.metadata_assignment_id "
        "WHERE v.backend_id=? AND a.assignment_state='selected' "
        "AND a.manual_assignment=1;";

    sqlite3_stmt* statement = nullptr;
    if (sqlite3_prepare_v2(
            database_.handle(),
            cacheAvailable ? sqlWithCache : sqlWithoutCache,
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

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const std::string lookupKey = columnText(statement, 0);
        const ManualRecordingMetadataAssignment assignment =
            readManualAssignment(statement, 1);
        if (!lookupKey.empty()) result[lookupKey] = assignment;
        if (!assignment.resourceKey.empty())
            result[assignment.resourceKey] = assignment;
    }

    sqlite3_finalize(statement);
    return result;
}
