#include "DeviceMediaProfileRepository.h"
#include "Database.h"

#include <cassert>
int main()
{
    Database db;
    assert(db.open(":memory:"));
    assert(db.execute("CREATE TABLE security_devices (device_id TEXT PRIMARY KEY,"
        "actor_id TEXT NOT NULL,display_name TEXT NOT NULL,active INTEGER NOT NULL,"
        "revoked_at TEXT NOT NULL);"));
    assert(db.execute("INSERT INTO security_devices VALUES "
        "('dev1','actor1','Television',1,'');"));
    DeviceMediaProfileRepository store(db);
    assert(store.ensureSchema());
    assert(store.ensureSchema());

    DeviceMediaProfileKey key{"dev1","b24","vidaa9","mse",
        MediaPlaybackEngine::MediaSourceExtensions};
    MediaPlaybackPathEvidence p;
    p.engine = key.engine;
    p.grade = MediaEvidenceGrade::RuntimeReported;
    p.protocol = MediaDeliveryProtocol::Hls;
    p.container = MediaContainer::Fmp4;
    p.videoCodec = MediaCodec::H264;
    p.audioCodec = MediaCodec::Aac;
    p.pictureObserved = true; p.soundObserved = true;
    p.verifiedVideoWidth = 1920;
    p.verifiedVideoHeight = 1080;
    p.verifiedAudioChannels = 2;
    assert(!store.upsertCorroboratedPlayback(key,p));
    p.grade = MediaEvidenceGrade::VerifiedPlayback;
    assert(store.upsertCorroboratedPlayback(key,p));
    auto found = store.findVerified(key);
    assert(found && found->verifiedPaths.size() == 1);
    assert(found->verifiedPaths[0].videoCodec == MediaCodec::H264);
    assert(store.upsertCorroboratedPlayback(key,p));
    found = store.findVerified(key);
    assert(found && found->verifiedPaths.size() == 1);
    auto other = key;
    other.deviceId = "dev2";
    assert(!store.upsertCorroboratedPlayback(other,p));
    assert(store.findVerified(other) && store.findVerified(other)->verifiedPaths.empty());
    other = key;
    other.appVersion = "b25";
    assert(store.findVerified(other) && store.findVerified(other)->verifiedPaths.empty());
    other = key;
    other.firmwareVersion = "vidaa10";
    assert(store.findVerified(other) && store.findVerified(other)->verifiedPaths.empty());
    assert(db.execute("UPDATE security_devices SET revoked_at='2026-10-12' WHERE device_id='dev1';"));
    assert(!store.upsertCorroboratedPlayback(key,p));
    assert(store.findVerified(key) && store.findVerified(key)->verifiedPaths.empty());
}
