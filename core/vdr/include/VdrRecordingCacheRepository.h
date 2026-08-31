#pragma once

#include "VdrRecording.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

class Database;

struct VdrRecordingCacheStatus
{
    std::string backendId;
    std::string state = "empty";
    int totalCount = 0;
    std::string startedAt;
    std::string finishedAt;
    std::string lastError;
};

struct VdrRecordingFolderEntry
{
    std::string name;
    std::string path;
    int recordingCount = 0;
    bool singleRecordingLeaf = false;
    VdrRecording singleRecording;
    bool representativeRecordingAvailable = false;
    VdrRecording representativeRecording;
};

struct VdrRecordingFolderPage
{
    std::string backendId;
    std::string path;
    std::string parentPath;
    std::string cacheState = "empty";
    bool cacheReady = false;
    int totalCount = 0;
    int folderCount = 0;
    int recordingCount = 0;
    int limit = 50;
    int offset = 0;
    std::vector<VdrRecordingFolderEntry> folders;
    std::vector<VdrRecording> recordings;
};

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

    bool markRefreshStarted(
        const std::string& backendId);

    bool markRefreshFinished(
        const std::string& backendId,
        int totalCount);

    bool markRefreshFailed(
        const std::string& backendId,
        const std::string& errorMessage);

    VdrRecordingCacheStatus statusForBackend(
        const std::string& backendId) const;

    bool warmBrowseSnapshotForBackend(
        const std::string& backendId) const;

    VdrRecordingFolderPage folderPageForBackend(
        const std::string& backendId,
        const std::string& folderPath,
        int limit,
        int offset) const;

private:
    struct BrowseFolderSnapshot
    {
        std::vector<VdrRecordingFolderEntry> folders;
        std::vector<VdrRecording> recordings;
    };

    struct BrowseBackendSnapshot
    {
        int totalCount = 0;
        std::map<std::string, BrowseFolderSnapshot> folders;
    };

    Database& database_;
    mutable std::recursive_mutex mutex_;
    mutable std::map<std::string, BrowseBackendSnapshot> browseSnapshots_;

    static std::string normalizeBackendId(
        const std::string& backendId);

    static std::string normalizeFolderPath(
        const std::string& folderPath);

    static std::string parentFolderPath(
        const std::string& folderPath);

    static std::string cacheKeyForRecording(
        const VdrRecording& recording);

    static std::string folderPathForRecording(
        const VdrRecording& recording);

    bool upsertRecordingsForBackendLocked(
        const std::string& normalizedBackendId,
        const std::vector<VdrRecording>& recordings);

    void storeBrowseSnapshotLocked(
        const std::string& normalizedBackendId,
        const std::vector<VdrRecording>& recordings) const;

    bool rebuildBrowseSnapshotFromPersistentCacheLocked(
        const std::string& normalizedBackendId) const;
};
