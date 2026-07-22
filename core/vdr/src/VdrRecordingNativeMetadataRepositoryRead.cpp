#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

using namespace vdr_recording_native_repository_detail;

VdrRecordingNativeMetadataRecord VdrRecordingNativeMetadataRepository::find(
    const std::string& backendId,
    const std::string& recordingKey) const
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    VdrRecordingNativeMetadataRecord record;
    if (!validRecordingKey(recordingKey) || !ensureSchemaLocked()) return record;

    const std::string normalizedBackendId = normalizeBackendId(backendId);
    Statement statement(database_.handle(),
        "SELECT backend_native_id, content_state, last_attempt_state, schema_version, identity_schema, reason, provider, media_type, "
        "provider_id, season_number, episode_number, absolute_episode_number, runtime_minutes, duration_deviation_minutes, scraper_hd, "
        "scraper_language, popularity, vote_average, vote_count, adult, collection_id, last_season, title, original_title, episode_name, "
        "tagline, overview, release_date, first_aired, imdb_id, provider_status, collection_name, preferred_artwork_provider, "
        "preferred_artwork_path, preferred_artwork_width, preferred_artwork_height, resolved_at, expires_at, negative_expires_at, "
        "retry_count, next_retry_at, last_error FROM vdr_recording_native_metadata WHERE backend_id = ? AND recording_key = ?;");
    if (!statement.valid() || !bindText(statement.get(), 1, normalizedBackendId) || !bindText(statement.get(), 2, recordingKey) ||
        sqlite3_step(statement.get()) != SQLITE_ROW) return record;

    record.backendId = normalizedBackendId;
    record.recordingKey = recordingKey;
    record.backendNativeId = columnText(statement.get(), 0);
    record.contentState = columnText(statement.get(), 1);
    record.lastAttemptState = columnText(statement.get(), 2);
    auto& metadata = record.metadata;
    metadata.schema = sqlite3_column_int(statement.get(), 3);
    metadata.recordingIdentitySchema = sqlite3_column_int(statement.get(), 4);
    metadata.reason = columnText(statement.get(), 5);
    metadata.provider = columnText(statement.get(), 6);
    metadata.mediaType = columnText(statement.get(), 7);
    metadata.providerId = sqlite3_column_int(statement.get(), 8);
    metadata.seasonNumber = sqlite3_column_int(statement.get(), 9);
    metadata.episodeNumber = sqlite3_column_int(statement.get(), 10);
    metadata.absoluteEpisodeNumber = sqlite3_column_int(statement.get(), 11);
    metadata.runtimeMinutes = sqlite3_column_int(statement.get(), 12);
    metadata.durationDeviationMinutes = sqlite3_column_int(statement.get(), 13);
    metadata.scraperHd = sqlite3_column_int(statement.get(), 14);
    metadata.scraperLanguage = sqlite3_column_int(statement.get(), 15);
    metadata.popularity = sqlite3_column_double(statement.get(), 16);
    metadata.voteAverage = sqlite3_column_double(statement.get(), 17);
    metadata.voteCount = sqlite3_column_int(statement.get(), 18);
    metadata.adult = sqlite3_column_int(statement.get(), 19) != 0;
    metadata.collectionId = sqlite3_column_int(statement.get(), 20);
    metadata.lastSeason = sqlite3_column_int(statement.get(), 21);
    metadata.title = columnText(statement.get(), 22);
    metadata.originalTitle = columnText(statement.get(), 23);
    metadata.episodeName = columnText(statement.get(), 24);
    metadata.tagline = columnText(statement.get(), 25);
    metadata.overview = columnText(statement.get(), 26);
    metadata.releaseDate = columnText(statement.get(), 27);
    metadata.firstAired = columnText(statement.get(), 28);
    metadata.imdbId = columnText(statement.get(), 29);
    metadata.status = columnText(statement.get(), 30);
    metadata.collectionName = columnText(statement.get(), 31);
    metadata.preferredArtwork.provider = columnText(statement.get(), 32);
    metadata.preferredArtwork.path = columnText(statement.get(), 33);
    metadata.preferredArtwork.width = sqlite3_column_int(statement.get(), 34);
    metadata.preferredArtwork.height = sqlite3_column_int(statement.get(), 35);
    metadata.preferredArtwork.available = !metadata.preferredArtwork.path.empty();
    record.resolvedAt = sqlite3_column_int64(statement.get(), 36);
    record.expiresAt = sqlite3_column_int64(statement.get(), 37);
    record.negativeExpiresAt = sqlite3_column_int64(statement.get(), 38);
    record.retryCount = sqlite3_column_int(statement.get(), 39);
    record.nextRetryAt = sqlite3_column_int64(statement.get(), 40);
    record.lastError = columnText(statement.get(), 41);

    metadata.recordingKey = recordingKey;
    metadata.found = record.contentState == "found";
    metadata.availability = availabilityFromState(record.contentState, record.lastAttemptState);
    metadata.genres = loadTextList(database_.handle(), normalizedBackendId, recordingKey, "genre");
    metadata.productionCountries = loadTextList(database_.handle(), normalizedBackendId, recordingKey, "country");
    metadata.networks = loadTextList(database_.handle(), normalizedBackendId, recordingKey, "network");
    metadata.people = loadPeople(database_.handle(), normalizedBackendId, recordingKey);
    metadata.images = loadImages(database_.handle(), normalizedBackendId, recordingKey);
    return record;
}
