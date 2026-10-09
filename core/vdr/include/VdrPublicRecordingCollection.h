#pragma once

#include "VdrPublicRecordingIdentityRepository.h"
#include "VdrRecording.h"

#include <string>
#include <vector>

struct VdrPublicRecordingSummary
{
    std::string recordingId;
    std::string backendId;
    std::string title;
    std::string recordedAt;
    int durationSeconds = 0;
    bool durationKnown = false;
};

struct VdrPublicRecordingCollection
{
    std::vector<VdrPublicRecordingSummary> items;
    bool valid = true;
};

// Caller must supply all *already authorized* backend identifiers.
// Native paths, backend-native IDs and artwork-provider URLs never enter
// the projected public objects.
VdrPublicRecordingCollection projectPublicRecordings(
    const std::vector<VdrRecording>& cachedRecordings,
    const std::vector<VdrPublicRecordingBinding>& activeBindings,
    const std::vector<std::string>& authorizedBackendIds);

std::string serializePublicRecordingCollection(
    const VdrPublicRecordingCollection& collection);
