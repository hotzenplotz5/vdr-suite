#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

using namespace vdr_recording_native_repository_detail;

bool VdrRecordingNativeMetadataRepository::storeResolution(
    const std::string& backendId,
    const std::string& backendNativeId,
    const VdrRecordingNativeMetadata& metadata,
    std::int64_t resolvedAt,
    std::int64_t expiresAt,
    std::int64_t negativeExpiresAt)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || backendNativeId.empty() ||
        !validRecordingKey(metadata.recordingKey) ||
        (metadata.availability != VdrRecordingNativeMetadataAvailability::Found &&
         metadata.availability != VdrRecordingNativeMetadataAvailability::NotFound))
    {
        return false;
    }

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    const std::string contentState = metadata.found ? "found" : "not_found";
    Transaction transaction(database_);
    if (!transaction.active()) return false;

    Statement statement(
        database_.handle(),
        "INSERT INTO vdr_recording_native_metadata ("
        "backend_id, recording_key, backend_native_id, content_state, last_attempt_state, schema_version, identity_schema, reason, "
        "provider, media_type, provider_id, season_number, episode_number, absolute_episode_number, runtime_minutes, duration_deviation_minutes, "
        "scraper_hd, scraper_language, popularity, vote_average, vote_count, adult, collection_id, last_season, title, original_title, "
        "episode_name, tagline, overview, release_date, first_aired, imdb_id, provider_status, collection_name, preferred_artwork_provider, "
        "preferred_artwork_path, preferred_artwork_width, preferred_artwork_height, resolved_at, expires_at, negative_expires_at, "
        "retry_count, next_retry_at, last_error, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 0, 0, '', CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, recording_key) DO UPDATE SET "
        "backend_native_id=excluded.backend_native_id, content_state=excluded.content_state, last_attempt_state=excluded.last_attempt_state, "
        "schema_version=excluded.schema_version, identity_schema=excluded.identity_schema, reason=excluded.reason, provider=excluded.provider, "
        "media_type=excluded.media_type, provider_id=excluded.provider_id, season_number=excluded.season_number, episode_number=excluded.episode_number, "
        "absolute_episode_number=excluded.absolute_episode_number, runtime_minutes=excluded.runtime_minutes, "
        "duration_deviation_minutes=excluded.duration_deviation_minutes, scraper_hd=excluded.scraper_hd, scraper_language=excluded.scraper_language, "
        "popularity=excluded.popularity, vote_average=excluded.vote_average, vote_count=excluded.vote_count, adult=excluded.adult, "
        "collection_id=excluded.collection_id, last_season=excluded.last_season, title=excluded.title, original_title=excluded.original_title, "
        "episode_name=excluded.episode_name, tagline=excluded.tagline, overview=excluded.overview, release_date=excluded.release_date, "
        "first_aired=excluded.first_aired, imdb_id=excluded.imdb_id, provider_status=excluded.provider_status, collection_name=excluded.collection_name, "
        "preferred_artwork_provider=excluded.preferred_artwork_provider, preferred_artwork_path=excluded.preferred_artwork_path, "
        "preferred_artwork_width=excluded.preferred_artwork_width, preferred_artwork_height=excluded.preferred_artwork_height, "
        "resolved_at=excluded.resolved_at, expires_at=excluded.expires_at, negative_expires_at=excluded.negative_expires_at, "
        "retry_count=0, next_retry_at=0, last_error='', updated_at=CURRENT_TIMESTAMP;");
    if (!statement.valid()) return false;

    int index = 1;
    const bool bound =
        bindText(statement.get(), index++, normalizedBackendId) && bindText(statement.get(), index++, metadata.recordingKey) &&
        bindText(statement.get(), index++, backendNativeId) && bindText(statement.get(), index++, contentState) &&
        bindText(statement.get(), index++, availabilityText(metadata.availability)) && sqlite3_bind_int(statement.get(), index++, metadata.schema) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.recordingIdentitySchema) == SQLITE_OK && bindText(statement.get(), index++, metadata.reason) &&
        bindText(statement.get(), index++, metadata.provider) && bindText(statement.get(), index++, metadata.mediaType) &&
        sqlite3_bind_int(statement.get(), index++, metadata.providerId) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.seasonNumber) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.episodeNumber) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.absoluteEpisodeNumber) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.runtimeMinutes) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.durationDeviationMinutes) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.scraperHd) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.scraperLanguage) == SQLITE_OK &&
        sqlite3_bind_double(statement.get(), index++, metadata.popularity) == SQLITE_OK && sqlite3_bind_double(statement.get(), index++, metadata.voteAverage) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.voteCount) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.adult ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.collectionId) == SQLITE_OK && sqlite3_bind_int(statement.get(), index++, metadata.lastSeason) == SQLITE_OK &&
        bindText(statement.get(), index++, metadata.title) && bindText(statement.get(), index++, metadata.originalTitle) &&
        bindText(statement.get(), index++, metadata.episodeName) && bindText(statement.get(), index++, metadata.tagline) &&
        bindText(statement.get(), index++, metadata.overview) && bindText(statement.get(), index++, metadata.releaseDate) &&
        bindText(statement.get(), index++, metadata.firstAired) && bindText(statement.get(), index++, metadata.imdbId) &&
        bindText(statement.get(), index++, metadata.status) && bindText(statement.get(), index++, metadata.collectionName) &&
        bindText(statement.get(), index++, metadata.preferredArtwork.provider) && bindText(statement.get(), index++, metadata.preferredArtwork.path) &&
        sqlite3_bind_int(statement.get(), index++, metadata.preferredArtwork.width) == SQLITE_OK &&
        sqlite3_bind_int(statement.get(), index++, metadata.preferredArtwork.height) == SQLITE_OK &&
        sqlite3_bind_int64(statement.get(), index++, resolvedAt) == SQLITE_OK && sqlite3_bind_int64(statement.get(), index++, expiresAt) == SQLITE_OK &&
        sqlite3_bind_int64(statement.get(), index++, negativeExpiresAt) == SQLITE_OK;

    if (!bound || sqlite3_step(statement.get()) != SQLITE_DONE ||
        !deleteChildren(database_.handle(), normalizedBackendId, metadata.recordingKey) ||
        !insertTextList(database_.handle(), normalizedBackendId, metadata.recordingKey, "genre", metadata.genres) ||
        !insertTextList(database_.handle(), normalizedBackendId, metadata.recordingKey, "country", metadata.productionCountries) ||
        !insertTextList(database_.handle(), normalizedBackendId, metadata.recordingKey, "network", metadata.networks) ||
        !insertPeople(database_.handle(), normalizedBackendId, metadata.recordingKey, metadata.people) ||
        !insertImages(database_.handle(), normalizedBackendId, metadata.recordingKey, metadata.images))
    {
        return false;
    }
    return transaction.commit();
}

bool VdrRecordingNativeMetadataRepository::recordFailure(
    const std::string& backendId,
    const std::string& backendNativeId,
    const std::string& recordingKey,
    VdrRecordingNativeMetadataAvailability failure,
    const std::string& diagnostic,
    int retryCount,
    std::int64_t nextRetryAt)
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!ensureSchemaLocked() || backendNativeId.empty() || !validRecordingKey(recordingKey) || retryCount < 0 ||
        (failure != VdrRecordingNativeMetadataAvailability::ProviderUnavailable &&
         failure != VdrRecordingNativeMetadataAvailability::TransportError &&
         failure != VdrRecordingNativeMetadataAvailability::InvalidPayload))
    {
        return false;
    }

    Statement statement(database_.handle(),
        "INSERT INTO vdr_recording_native_metadata (backend_id, recording_key, backend_native_id, content_state, last_attempt_state, retry_count, next_retry_at, last_error, updated_at) "
        "VALUES (?, ?, ?, 'empty', ?, ?, ?, ?, CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id, recording_key) DO UPDATE SET backend_native_id=excluded.backend_native_id, "
        "last_attempt_state=excluded.last_attempt_state, retry_count=excluded.retry_count, next_retry_at=excluded.next_retry_at, "
        "last_error=excluded.last_error, updated_at=CURRENT_TIMESTAMP;");
    if (!statement.valid()) return false;

    const bool bound = bindText(statement.get(), 1, normalizeBackendId(backendId)) && bindText(statement.get(), 2, recordingKey) &&
        bindText(statement.get(), 3, backendNativeId) && bindText(statement.get(), 4, availabilityText(failure)) &&
        sqlite3_bind_int(statement.get(), 5, retryCount) == SQLITE_OK && sqlite3_bind_int64(statement.get(), 6, nextRetryAt) == SQLITE_OK &&
        bindText(statement.get(), 7, diagnostic);
    return bound && sqlite3_step(statement.get()) == SQLITE_DONE;
}
