#include "DeviceMediaProfileRepository.h"

#include "Database.h"

#include <sqlite3.h>

namespace
{
bool bind(sqlite3_stmt* stmt, int index, const std::string& value)
{
    return sqlite3_bind_text(stmt, index, value.c_str(), -1,
        SQLITE_TRANSIENT) == SQLITE_OK;
}

bool bindKey(sqlite3_stmt* stmt, const DeviceMediaProfileKey& key)
{
    return bind(stmt, 1, key.deviceId) &&
        bind(stmt, 2, key.appVersion) &&
        bind(stmt, 3, key.firmwareVersion) &&
        bind(stmt, 4, key.playerVersion) &&
        sqlite3_bind_int(stmt, 5, static_cast<int>(key.engine)) == SQLITE_OK;
}

std::string columnString(sqlite3_stmt* stmt, int index)
{
    const auto* text = sqlite3_column_text(stmt, index);
    return text == nullptr ? std::string() :
        std::string(reinterpret_cast<const char*>(text));
}

bool validCodec(MediaCodec codec)
{
    switch (codec) {
        case MediaCodec::H264: case MediaCodec::H265:
        case MediaCodec::Mpeg2Video: case MediaCodec::Aac:
        case MediaCodec::Ac3: case MediaCodec::Eac3:
        case MediaCodec::MpegAudio: case MediaCodec::Dts:
            return true;
        default: return false;
    }
}

bool validEvidence(
    const DeviceMediaProfileKey& key,
    const MediaPlaybackPathEvidence& p)
{
    if (!key.valid() || p.engine != key.engine ||
        p.grade != MediaEvidenceGrade::VerifiedPlayback ||
        !p.pictureObserved || !p.soundObserved ||
        !validCodec(p.videoCodec) || !validCodec(p.audioCodec) ||
        p.verifiedVideoWidth <= 0 || p.verifiedVideoWidth > 16384 ||
        p.verifiedVideoHeight <= 0 || p.verifiedVideoHeight > 16384 ||
        p.verifiedAudioChannels <= 0 || p.verifiedAudioChannels > 32 ||
        p.videoProfile.size() > 32 || p.hdrTransfer.size() > 32 ||
        p.verifiedVideoBitDepth < 0 || p.verifiedVideoBitDepth > 16 ||
        p.verifiedMaxFrameRate < 0.0 || p.verifiedMaxFrameRate > 240.0)
        return false;
    return p.container == MediaContainer::MpegTs ||
           p.container == MediaContainer::Mp4 ||
           p.container == MediaContainer::Fmp4;
}
} // namespace

DeviceMediaProfileRepository::DeviceMediaProfileRepository(Database& database)
    : database_(database) {}

bool DeviceMediaProfileRepository::ensureSchema()
{
    return database_.execute(
        "CREATE TABLE IF NOT EXISTS device_media_verified_paths ("
        "device_id TEXT NOT NULL REFERENCES security_devices(device_id),"
        "app_version TEXT NOT NULL,"
        "firmware_version TEXT NOT NULL,"
        "player_version TEXT NOT NULL,"
        "engine INTEGER NOT NULL,"
        "protocol INTEGER NOT NULL,"
        "container INTEGER NOT NULL,"
        "video_codec INTEGER NOT NULL,"
        "audio_codec INTEGER NOT NULL,"
        "max_width INTEGER NOT NULL,"
        "max_height INTEGER NOT NULL,"
        "max_channels INTEGER NOT NULL,"
        "video_profile TEXT NOT NULL,"
        "video_bit_depth INTEGER NOT NULL,"
        "max_fps REAL NOT NULL,"
        "hdr_transfer TEXT NOT NULL,"
        "hdr_output INTEGER NOT NULL,"
        "range_http INTEGER NOT NULL,"
        "contract_version INTEGER NOT NULL,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(device_id,app_version,firmware_version,player_version,"
        "engine,protocol,container,video_codec,audio_codec,video_profile,"
        "video_bit_depth,hdr_transfer)"
        ");");
}

bool DeviceMediaProfileRepository::upsertCorroboratedPlayback(
    const DeviceMediaProfileKey& key,
    const MediaPlaybackPathEvidence& p)
{
    if (!validEvidence(key, p)) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "INSERT INTO device_media_verified_paths "
        "(device_id,app_version,firmware_version,player_version,engine,"
        "protocol,container,video_codec,audio_codec,max_width,max_height,"
        "max_channels,video_profile,video_bit_depth,max_fps,hdr_transfer,"
        "hdr_output,range_http,contract_version) "
        "SELECT ?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,1 "
        "WHERE EXISTS (SELECT 1 FROM security_devices "
        "WHERE device_id=? AND active=1 AND revoked_at='') "
        "AND (SELECT COUNT(*) FROM device_media_verified_paths "
        "WHERE device_id=? AND app_version=? AND firmware_version=? "
        "AND player_version=? AND engine=?) < 32 "
        "ON CONFLICT DO UPDATE SET "
        "max_width=excluded.max_width,max_height=excluded.max_height,"
        "max_channels=excluded.max_channels,max_fps=excluded.max_fps,"
        "hdr_output=excluded.hdr_output,range_http=excluded.range_http,"
        "updated_at=CURRENT_TIMESTAMP;";
    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    const bool ok = bindKey(stmt,key) &&
        sqlite3_bind_int(stmt,6,static_cast<int>(p.protocol)) == SQLITE_OK &&
        sqlite3_bind_int(stmt,7,static_cast<int>(p.container)) == SQLITE_OK &&
        sqlite3_bind_int(stmt,8,static_cast<int>(p.videoCodec)) == SQLITE_OK &&
        sqlite3_bind_int(stmt,9,static_cast<int>(p.audioCodec)) == SQLITE_OK &&
        sqlite3_bind_int(stmt,10,p.verifiedVideoWidth) == SQLITE_OK &&
        sqlite3_bind_int(stmt,11,p.verifiedVideoHeight) == SQLITE_OK &&
        sqlite3_bind_int(stmt,12,p.verifiedAudioChannels) == SQLITE_OK &&
        bind(stmt,13,p.videoProfile) &&
        sqlite3_bind_int(stmt,14,p.verifiedVideoBitDepth) == SQLITE_OK &&
        sqlite3_bind_double(stmt,15,p.verifiedMaxFrameRate) == SQLITE_OK &&
        bind(stmt,16,p.hdrTransfer) &&
        sqlite3_bind_int(stmt,17,p.hdrOutputObserved ? 1 : 0) == SQLITE_OK &&
        sqlite3_bind_int(stmt,18,p.byteRangeHttpObserved ? 1 : 0) == SQLITE_OK &&
        bind(stmt,19,key.deviceId) && bind(stmt,20,key.deviceId) &&
        bind(stmt,21,key.appVersion) && bind(stmt,22,key.firmwareVersion) &&
        bind(stmt,23,key.playerVersion) &&
        sqlite3_bind_int(stmt,24,static_cast<int>(key.engine)) == SQLITE_OK;
    const bool saved = ok && sqlite3_step(stmt) == SQLITE_DONE &&
        sqlite3_changes(database_.handle()) != 0;
    sqlite3_finalize(stmt);
    return saved;
}

std::optional<DeviceMediaCapabilityProfile> DeviceMediaProfileRepository::findVerified(
    const DeviceMediaProfileKey& key) const
{
    if (!key.valid()) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    const char* sql =
        "SELECT p.protocol,p.container,p.video_codec,p.audio_codec,"
        "p.max_width,p.max_height,p.max_channels,p.video_profile,"
        "p.video_bit_depth,p.max_fps,p.hdr_transfer,p.hdr_output,p.range_http "
        "FROM device_media_verified_paths AS p "
        "JOIN security_devices AS d ON d.device_id=p.device_id "
        "WHERE p.device_id=? AND p.app_version=? AND p.firmware_version=? "
        "AND p.player_version=? AND p.engine=? AND p.contract_version=1 "
        "AND d.active=1 AND d.revoked_at='' LIMIT 33;";
    if (sqlite3_prepare_v2(database_.handle(),sql,-1,&stmt,nullptr) != SQLITE_OK)
        return std::nullopt;
    if (!bindKey(stmt,key)) {
        sqlite3_finalize(stmt);
        return std::nullopt;
    }
    DeviceMediaCapabilityProfile profile;
    profile.key = key;
    int step = SQLITE_ROW;
    while ((step = sqlite3_step(stmt)) == SQLITE_ROW) {
        MediaPlaybackPathEvidence p;
        p.engine = key.engine;
        p.grade = MediaEvidenceGrade::VerifiedPlayback;
        p.protocol = static_cast<MediaDeliveryProtocol>(sqlite3_column_int(stmt,0));
        p.container = static_cast<MediaContainer>(sqlite3_column_int(stmt,1));
        p.videoCodec = static_cast<MediaCodec>(sqlite3_column_int(stmt,2));
        p.audioCodec = static_cast<MediaCodec>(sqlite3_column_int(stmt,3));
        p.verifiedVideoWidth = sqlite3_column_int(stmt,4);
        p.verifiedVideoHeight = sqlite3_column_int(stmt,5);
        p.verifiedAudioChannels = sqlite3_column_int(stmt,6);
        p.videoProfile = columnString(stmt,7);
        p.verifiedVideoBitDepth = sqlite3_column_int(stmt,8);
        p.verifiedMaxFrameRate = sqlite3_column_double(stmt,9);
        p.hdrTransfer = columnString(stmt,10);
        p.hdrOutputObserved = sqlite3_column_int(stmt,11) != 0;
        p.byteRangeHttpObserved = sqlite3_column_int(stmt,12) != 0;
        p.pictureObserved = true;
        p.soundObserved = true;
        if (!validEvidence(key,p) || profile.verifiedPaths.size() == 32U) {
            sqlite3_finalize(stmt);
            return std::nullopt;
        }
        profile.verifiedPaths.push_back(p);
    }
    sqlite3_finalize(stmt);
    if (step != SQLITE_DONE) return std::nullopt;
    return profile;
}
