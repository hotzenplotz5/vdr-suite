#include "MediaTranscodeBackendSettingsService.h"

#include <algorithm>
#include <cstdlib>
#include <sqlite3.h>
#include <utility>

namespace
{
bool bindText(sqlite3_stmt* statement, int index, const std::string& value)
{
    return sqlite3_bind_text(
        statement,
        index,
        value.c_str(),
        -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

std::string columnText(sqlite3_stmt* statement, int column)
{
    const unsigned char* value = sqlite3_column_text(statement, column);
    return value == nullptr
        ? std::string()
        : reinterpret_cast<const char*>(value);
}

std::optional<MediaVideoEncoderMode> parsedMode(const std::string& value)
{
    bool valid = false;
    const MediaVideoEncoderMode mode =
        MediaTranscodePolicy::videoEncoderModeFromString(value, valid);
    return valid ? std::optional<MediaVideoEncoderMode>(mode) : std::nullopt;
}
}

MediaTranscodeBackendSettingsService::MediaTranscodeBackendSettingsService(
    Database& database,
    std::string backendId)
    : database_(database),
      backendId_(std::move(backendId))
{
}

bool MediaTranscodeBackendSettingsService::validBackendId(
    const std::string& backendId)
{
    return !backendId.empty() && backendId.size() <= 128U &&
        std::all_of(
            backendId.begin(),
            backendId.end(),
            [](unsigned char character)
            {
                return (character >= 'a' && character <= 'z') ||
                    (character >= 'A' && character <= 'Z') ||
                    (character >= '0' && character <= '9') ||
                    character == '-' || character == '_' || character == '.';
            });
}

bool MediaTranscodeBackendSettingsService::validManagedMode(
    const std::string& mode)
{
    return mode == "auto" || mode == "software" || mode == "vaapi";
}

bool MediaTranscodeBackendSettingsService::ensureSchema()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return validBackendId(backendId_) && ensureSchemaLocked();
}

bool MediaTranscodeBackendSettingsService::ensureSchemaLocked() const
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS backend_media_transcode_settings ("
        "backend_id TEXT PRIMARY KEY,"
        "video_encoder_mode TEXT NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "CHECK(video_encoder_mode IN ('auto','software','vaapi'))"
        ");");
}

bool MediaTranscodeBackendSettingsService::readManagedModeLocked(
    std::string& mode) const
{
    mode.clear();
    if (!ensureSchemaLocked()) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "SELECT video_encoder_mode FROM backend_media_transcode_settings "
        "WHERE backend_id=? LIMIT 1;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool bound = bindText(statement, 1, backendId_);
    const int step = bound ? sqlite3_step(statement) : SQLITE_ERROR;
    if (step == SQLITE_ROW) mode = columnText(statement, 0);
    sqlite3_finalize(statement);
    return step == SQLITE_ROW || step == SQLITE_DONE;
}

bool MediaTranscodeBackendSettingsService::storeManagedModeLocked(
    const std::string& mode) const
{
    if (!ensureSchemaLocked() || !validManagedMode(mode)) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "INSERT INTO backend_media_transcode_settings "
        "(backend_id,video_encoder_mode,updated_at) "
        "VALUES (?,?,CURRENT_TIMESTAMP) "
        "ON CONFLICT(backend_id) DO UPDATE SET "
        "video_encoder_mode=excluded.video_encoder_mode,"
        "updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool stored =
        bindText(statement, 1, backendId_) &&
        bindText(statement, 2, mode) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return stored;
}

bool MediaTranscodeBackendSettingsService::clearManagedModeLocked() const
{
    if (!ensureSchemaLocked()) return false;

    sqlite3_stmt* statement = nullptr;
    const char* sql =
        "DELETE FROM backend_media_transcode_settings WHERE backend_id=?;";
    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr) != SQLITE_OK)
    {
        return false;
    }

    const bool removed =
        bindText(statement, 1, backendId_) &&
        sqlite3_step(statement) == SQLITE_DONE;
    sqlite3_finalize(statement);
    return removed;
}

MediaTranscodePolicy MediaTranscodeBackendSettingsService::resolvePolicyLocked(
    const std::optional<MediaVideoEncoderMode>& managedMode) const
{
    return MediaTranscodePolicy::fromEnvironment(managedMode);
}

MediaTranscodeBackendSettingsSnapshot
MediaTranscodeBackendSettingsService::snapshotLocked() const
{
    MediaTranscodeBackendSettingsSnapshot snapshot;
    snapshot.backendId = backendId_;

    std::string managedMode;
    const bool managedRead = readManagedModeLocked(managedMode);
    std::optional<MediaVideoEncoderMode> managed;
    if (managedRead && validManagedMode(managedMode)) {
        managed = parsedMode(managedMode);
    }

    snapshot.managed = managed.has_value();
    snapshot.managedMode = snapshot.managed ? managedMode : std::string();

    if (snapshot.managed) {
        snapshot.configurationSource = "managed";
    }
    else {
        const char* raw = std::getenv("VDR_SUITE_MEDIA_VIDEO_ENCODER");
        const std::optional<MediaVideoEncoderMode> environment =
            raw == nullptr ? std::nullopt : parsedMode(raw);
        snapshot.configurationSource = environment.has_value()
            ? "environment"
            : "default";
    }

    const MediaTranscodePolicy policy = resolvePolicyLocked(managed);
    snapshot.effectiveMode =
        MediaTranscodePolicy::videoEncoderModeName(policy.videoEncoderMode());
    snapshot.diagnostics = policy.diagnostics();
    return snapshot;
}

MediaTranscodeBackendSettingsSnapshot
MediaTranscodeBackendSettingsService::get() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId_)) return {};
    return snapshotLocked();
}

MediaTranscodeBackendSettingsUpdateResult
MediaTranscodeBackendSettingsService::update(
    const MediaTranscodeBackendSettingsUpdate& request)
{
    MediaTranscodeBackendSettingsUpdateResult result;
    if (request.backendId != backendId_ || !validBackendId(request.backendId)) {
        result.statusCode = 400;
        result.errorCode = "backend_id_mismatch";
        result.message = "The backend ID in the route and payload must match";
        return result;
    }

    if (request.clearManagedOverride) {
        if (!request.videoEncoderMode.empty()) {
            result.statusCode = 400;
            result.errorCode = "conflicting_encoder_setting";
            result.message = "Deployment default cannot be combined with a managed encoder mode";
            return result;
        }
    }
    else if (!validManagedMode(request.videoEncoderMode)) {
        result.statusCode = 400;
        result.errorCode = "invalid_video_encoder_mode";
        result.message = "The video encoder mode must be auto, software, or vaapi";
        return result;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    const bool stored = request.clearManagedOverride
        ? clearManagedModeLocked()
        : storeManagedModeLocked(request.videoEncoderMode);
    if (!stored) {
        result.statusCode = 503;
        result.errorCode = "media_transcode_settings_persistence_failed";
        result.message = "Media transcode settings could not be persisted";
        return result;
    }

    result.success = true;
    result.statusCode = 200;
    result.settings = snapshotLocked();
    return result;
}

MediaTranscodePolicy MediaTranscodeBackendSettingsService::resolvePolicy() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!validBackendId(backendId_)) return MediaTranscodePolicy::fromEnvironment();

    std::string managedMode;
    std::optional<MediaVideoEncoderMode> managed;
    if (readManagedModeLocked(managedMode) && validManagedMode(managedMode)) {
        managed = parsedMode(managedMode);
    }
    return resolvePolicyLocked(managed);
}
