#pragma once

#include "DeviceMediaCapabilityProfile.h"

#include <optional>

class Database;

// Server-owned SQLite persistence for exact, versioned, Device-specific
// playback evidence. Uses existing security_devices identity, not a parallel
// pairing/authorization scheme. The caller must authorize a Device identity
// before resolving the key. No public client can write VerifiedPlayback.
class DeviceMediaProfileRepository final
{
public:
    explicit DeviceMediaProfileRepository(Database& database);
    bool ensureSchema();

    // Internal-only input: MUST already have been correlated against an
    // authorized Session and actual emitted media, with a separately
    // corroborated decoded picture/sound. Never call from untrusted JSON.
    bool upsertCorroboratedPlayback(
        const DeviceMediaProfileKey& serverBoundKey,
        const MediaPlaybackPathEvidence& trustedObservation);

    // nullopt means unavailable/invalid, NOT a successful empty profile.
    std::optional<DeviceMediaCapabilityProfile> findVerified(
        const DeviceMediaProfileKey& serverBoundKey) const;

private:
    Database& database_;
};
