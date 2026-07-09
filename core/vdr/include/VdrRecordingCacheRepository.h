#pragma once

#include "VdrRecording.h"

#include <mutex>
#include <string>
#include <vector>

class Database;

class VdrRecordingCacheRepository
{
public:
    explicit VdrRecordingCacheRepository(
        Database& database);

    bool ensureSchema();

    bool upsertRecordingsForBackend(
        const std::string& backendId,
        const std::vector<VdrRecording>& recordings);

    bool replaceRecordingsForBackend(
        const std::string& backendId,
        const std::vector<VdrRecording>& recordings);

    std::vector<VdrRecording> findAllForBackend(
        const std::string& backendId) const;

    int countForBackend(
        const std::string& backendId) const;

private:
    Database& database_;
    mutable std::recursive_mutex mutex_;

    static std::string normalizeBackendId(
        const std::string& backendId);

    static std::string cacheKeyForRecording(
        const VdrRecording& recording);

    bool upsertRecordingsForBackendLocked(
        const std::string& normalizedBackendId,
        const std::vector<VdrRecording>& recordings);
};
