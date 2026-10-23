#pragma once

#include "MediaCapabilityEvidence.h"
#include "MediaPresentationSelector.h"

#include <string>
#include <vector>

// A versioned, Device-scoped, player-engine-specific set of individual
// playback paths. The Device ID MUST originate from the authenticated
// RequestSecurityContext, never from a client JSON field.
struct DeviceMediaProfileKey
{
    std::string deviceId;
    std::string appVersion;
    std::string firmwareVersion;
    std::string playerVersion;
    MediaPlaybackEngine engine = MediaPlaybackEngine::Native;

    bool valid() const;
    bool operator==(const DeviceMediaProfileKey& other) const;
};

struct DeviceMediaCapabilityProfile
{
    static constexpr unsigned ContractVersion = 1;
    unsigned version = ContractVersion;
    DeviceMediaProfileKey key;
    // These are trusted and corroborated *server-side* observations, not
    // submitted capability claims or results of canPlayType().
    std::vector<MediaPlaybackPathEvidence> verifiedPaths;
};

struct DeviceMediaSelection
{
    bool available = false;
    MediaPresentationProfile presentation;
    MediaPlaybackEngine engine = MediaPlaybackEngine::Native;
    std::string reason;
};

// Delegates all actual media choices to the existing Selector. Intersects
// each *single* verified output path with the untrusted advertisement,
// source descriptors and the app/firmware/player-scoped identity. Never
// unions separate paths into false codec/container combinations.
class DeviceMediaProfileSelector final
{
public:
    DeviceMediaSelection select(
        const MediaSourceDescriptor& trustedSource,
        const ClientMediaCapabilities& advertised,
        const DeviceMediaCapabilityProfile& storedProfile,
        const DeviceMediaProfileKey& authenticatedPlayer,
        int preferredAudioStreamIndex = -1) const;
};
