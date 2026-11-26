#pragma once

#include "DashboardController.h"
#include "PersonQuery.h"
#include "RecordingPersonSearchResult.h"
#include "VdrRecording.h"

#include <functional>
#include <string>
#include <vector>

class RecordingPersonSearchResultJsonSerializer;
class RecordingPersonSearchService;

class RecordingPersonSearchController
{
public:
    using PersistentSearch = std::function<
        RecordingPersonSearchResult(
            const std::string& backendId,
            const PersonQuery& query,
            int limit,
            int offset)>;

    using PublicRecordingIdLookup = std::function<std::string(
        const std::string& backendId, const std::string& backendNativeId)>;

    RecordingPersonSearchController(
        RecordingPersonSearchService& searchService,
        RecordingPersonSearchResultJsonSerializer& jsonSerializer,
        PersistentSearch persistentSearch = {},
        PublicRecordingIdLookup publicRecordingIdLookup = {});

    bool usesPersistentSearch() const noexcept;

    ApiResponse searchRecordingPersons(
        const std::vector<VdrRecording>& recordings,
        const std::string& name,
        const std::string& normalizedName,
        const std::string& characterName,
        const std::string& role,
        const std::string& source,
        const std::string& providerReference,
        int limit,
        int offset);

    ApiResponse searchRecordingPersons(
        const std::string& backendId,
        const std::vector<VdrRecording>& fallbackRecordings,
        const std::string& name,
        const std::string& normalizedName,
        const std::string& characterName,
        const std::string& role,
        const std::string& source,
        const std::string& providerReference,
        int limit,
        int offset);

private:
    ApiResponse searchRecordingPersonsInternal(
        const std::string& backendId,
        const std::vector<VdrRecording>& fallbackRecordings,
        const std::string& name,
        const std::string& normalizedName,
        const std::string& characterName,
        const std::string& role,
        const std::string& source,
        const std::string& providerReference,
        int limit,
        int offset);

    RecordingPersonSearchService& searchService_;
    RecordingPersonSearchResultJsonSerializer& jsonSerializer_;
    PersistentSearch persistentSearch_;
    PublicRecordingIdLookup publicRecordingIdLookup_;
};
