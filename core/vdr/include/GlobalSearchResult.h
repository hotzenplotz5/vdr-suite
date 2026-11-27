#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct GlobalSearchRecording
{
    std::string id;
    std::string backendId;
    std::string backendNativeId;
    std::string title;
    std::string subtitle;
    std::string path;
    std::string startTime;
    int durationSeconds = 0;
    long long sizeMb = 0;
    bool artworkAvailable = false;
    // Internal evidence from the existing joined metadata row. Never expose
    // the cache path or assignment revision as a public image selector.
    std::string preferredArtworkPath;
    int preferredArtworkWidth = 0, preferredArtworkHeight = 0;
    int preferredArtworkManualRevision = 0;
    bool manualArtworkSelected = false;
    std::string matchedPerson;
    std::string matchedRole;
    std::string matchReason;
    std::string personKey;
};

struct GlobalSearchEpgEvent
{
    std::string backendId;
    std::string channelId;
    std::string channelName;
    std::string eventId;
    std::string title;
    std::string subtitle;
    std::string description;
    std::string startTime;
    std::string endTime;
    int durationSeconds = 0;
    bool artworkAvailable = false;
    std::string matchedPerson;
    std::string matchedRole;
    std::string matchReason;
    std::string personKey;
};

struct GlobalSearchPersonSummary
{
    std::string name;
    std::string role;
    int recordingCount = 0;
    int epgCount = 0;
};

struct GlobalSearchPersonPortrait
{
    std::string name;
    std::string role;
    std::string backendNativeId;
    int index = -1;
    int assignmentRevision = 0;
    // Internal cached evidence only, never serialized as a public path.
    std::string personKey;
    std::string localPath;
};

struct GlobalSearchResult
{
    std::string query;
    std::string backendId;
    std::string status = "ready";
    int minimumQueryLength = 2;
    int limit = 20;
    int offset = 0;
    std::int64_t epgFrom = 0;
    std::int64_t epgUntil = 0;
    int recordingTotal = 0;
    int epgTotal = 0;
    bool recordingHasMore = false;
    bool epgHasMore = false;
    std::vector<GlobalSearchRecording> recordings;
    std::vector<GlobalSearchEpgEvent> epg;
    std::vector<GlobalSearchPersonSummary> people;
    std::vector<GlobalSearchPersonPortrait> portraits;
};