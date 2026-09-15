#pragma once

#include "Database.h"

#include <string>

struct RecordingSeriesHierarchyOverride
{
    bool available = false;

    std::string backendId;
    std::string recordingKey;

    // season | special | custom
    std::string groupType;

    // Only meaningful for groupType=season.
    int seasonNumber = 0;

    // Human-readable label for special/custom groups.
    std::string groupLabel;

    // Stable ordering between special/custom groups.
    int sortOrder = 0;

    // Optional presentation range for double/multi episodes.
    int episodeStart = 0;
    int episodeEnd = 0;

    int revision = 0;
};

struct RecordingSeriesHierarchyOverrideMutation
{
    std::string backendId;
    std::string recordingKey;

    std::string groupType;
    int seasonNumber = 0;
    std::string groupLabel;
    int sortOrder = 0;
    int episodeStart = 0;
    int episodeEnd = 0;

    int expectedRevision = 0;
};

struct RecordingSeriesHierarchyOverrideMutationResult
{
    bool success = false;
    int statusCode = 500;
    std::string errorCode;
    std::string message;

    RecordingSeriesHierarchyOverride value;
};

class RecordingSeriesHierarchyOverrideRepository
{
public:
    explicit RecordingSeriesHierarchyOverrideRepository(
        Database& database);

    bool ensureSchema();

    RecordingSeriesHierarchyOverride get(
        const std::string& backendId,
        const std::string& recordingKey) const;

    RecordingSeriesHierarchyOverrideMutationResult set(
        const RecordingSeriesHierarchyOverrideMutation& mutation);

    RecordingSeriesHierarchyOverrideMutationResult clear(
        const std::string& backendId,
        const std::string& recordingKey,
        int expectedRevision = 0);

private:
    Database& database_;
};
