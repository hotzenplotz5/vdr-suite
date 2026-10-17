#pragma once

#include "DeviceMediaCapabilityProfile.h"

#include <optional>
#include <string>

// Parse a bounded versioned Device capability status URL. The caller
// supplies the Device ID from an already authenticated SecurityHttpGate.
std::optional<DeviceMediaProfileKey> parseDeviceMediaProfileQuery(
    const std::string& requestTarget,
    const std::string& collectionPath,
    const std::string& authenticatedDeviceId);
