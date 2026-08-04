#include "VdrRecordingNativeMetadataRepository.h"
#include "VdrRecordingNativeMetadataRepositoryInternal.h"
#include "Database.h"
#include <sqlite3.h>

VdrRecordingNativeMetadataRepository::VdrRecordingNativeMetadataRepository(
    Database& database)
    : database_(database)
{
}

bool VdrRecordingNativeMetadataRepository::ensureSchema()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return ensureSchemaLocked();
}

bool VdrRecordingNativeMetadataRepository::ensureSchemaLocked() const
{
    if (schemaReady_) return true;

    const bool ready = database_.execute(
        "CREATE TABLE IF NOT EXISTS vdr_recording_native_metadata ("
        "backend_id TEXT NOT NULL,"
        "recording_key TEXT NOT NULL,"
        "backend_native_id TEXT NOT NULL,"
        "content_state TEXT NOT NULL DEFAULT 'empty',"
        "last_attempt_state TEXT NOT NULL DEFAULT 'never',"
        "schema_version INTEGER NOT NULL DEFAULT 0,"
        "identity_schema INTEGER NOT NULL DEFAULT 0,"
        "reason TEXT NOT NULL DEFAULT '',"
        "provider TEXT NOT NULL DEFAULT 'none',"
        "media_type TEXT NOT NULL DEFAULT 'none',"
        "provider_id INTEGER NOT NULL DEFAULT 0,"
        "season_number INTEGER NOT NULL DEFAULT 0,"
        "episode_number INTEGER NOT NULL DEFAULT 0,"
        "absolute_episode_number INTEGER NOT NULL DEFAULT 0,"
        "runtime_minutes INTEGER NOT NULL DEFAULT 0,"
        "duration_deviation_minutes INTEGER NOT NULL DEFAULT 0,"
        "scraper_hd INTEGER NOT NULL DEFAULT 0,"
        "scraper_language INTEGER NOT NULL DEFAULT 0,"
        "popularity REAL NOT NULL DEFAULT 0,"
        "vote_average REAL NOT NULL DEFAULT 0,"
        "vote_count INTEGER NOT NULL DEFAULT 0,"
        "adult INTEGER NOT NULL DEFAULT 0,"
        "collection_id INTEGER NOT NULL DEFAULT 0,"
        "last_season INTEGER NOT NULL DEFAULT 0,"
        "title TEXT NOT NULL DEFAULT '',"
        "original_title TEXT NOT NULL DEFAULT '',"
        "episode_name TEXT NOT NULL DEFAULT '',"
        "tagline TEXT NOT NULL DEFAULT '',"
        "overview TEXT NOT NULL DEFAULT '',"
        "release_date TEXT NOT NULL DEFAULT '',"
        "first_aired TEXT NOT NULL DEFAULT '',"
        "imdb_id TEXT NOT NULL DEFAULT '',"
        "provider_status TEXT NOT NULL DEFAULT '',"
        "collection_name TEXT NOT NULL DEFAULT '',"
        "preferred_artwork_provider TEXT NOT NULL DEFAULT 'none',"
        "preferred_artwork_path TEXT NOT NULL DEFAULT '',"
        "preferred_artwork_width INTEGER NOT NULL DEFAULT 0,"
        "preferred_artwork_height INTEGER NOT NULL DEFAULT 0,"
        "resolved_at INTEGER NOT NULL DEFAULT 0,"
        "expires_at INTEGER NOT NULL DEFAULT 0,"
        "negative_expires_at INTEGER NOT NULL DEFAULT 0,"
        "retry_count INTEGER NOT NULL DEFAULT 0,"
        "next_retry_at INTEGER NOT NULL DEFAULT 0,"
        "last_error TEXT NOT NULL DEFAULT '',"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY (backend_id, recording_key)"
        ");"
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_vdr_recording_native_metadata_native_id "
        "ON vdr_recording_native_metadata (backend_id, backend_native_id);"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_native_metadata_due "
        "ON vdr_recording_native_metadata (backend_id, content_state, expires_at, negative_expires_at, next_retry_at);"
        "CREATE TABLE IF NOT EXISTS vdr_recording_native_text_list ("
        "backend_id TEXT NOT NULL, recording_key TEXT NOT NULL, kind TEXT NOT NULL, ordinal INTEGER NOT NULL, value TEXT NOT NULL,"
        "PRIMARY KEY (backend_id, recording_key, kind, ordinal));"
        "CREATE TABLE IF NOT EXISTS vdr_recording_native_person ("
        "backend_id TEXT NOT NULL, recording_key TEXT NOT NULL, ordinal INTEGER NOT NULL, role TEXT NOT NULL, name TEXT NOT NULL,"
        "name_folded TEXT NOT NULL, normalized_name TEXT NOT NULL, character_name TEXT NOT NULL DEFAULT '',"
        "character_name_folded TEXT NOT NULL DEFAULT '', image_provider TEXT NOT NULL DEFAULT 'none', image_path TEXT NOT NULL DEFAULT '',"
        "image_width INTEGER NOT NULL DEFAULT 0, image_height INTEGER NOT NULL DEFAULT 0,"
        "PRIMARY KEY (backend_id, recording_key, ordinal));"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_native_person_name "
        "ON vdr_recording_native_person (backend_id, normalized_name);"
        "CREATE INDEX IF NOT EXISTS idx_vdr_recording_native_person_role "
        "ON vdr_recording_native_person (backend_id, role);"
        "CREATE TABLE IF NOT EXISTS vdr_recording_native_artwork ("
        "backend_id TEXT NOT NULL, recording_key TEXT NOT NULL, ordinal INTEGER NOT NULL, orientation TEXT NOT NULL, provider TEXT NOT NULL,"
        "path TEXT NOT NULL, width INTEGER NOT NULL DEFAULT 0, height INTEGER NOT NULL DEFAULT 0,"
        "PRIMARY KEY (backend_id, recording_key, ordinal));");

    if (ready) schemaReady_ = true;
    return ready;
}

std::string VdrRecordingNativeMetadataRepository::normalizeBackendId(
    const std::string& backendId)
{
    return backendId.empty() ? "default" : backendId;
}
