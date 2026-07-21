#pragma once

#include "EpgPersonIndex.h"
#include "RecordingPersonSearchResult.h"

#include <string>
#include <vector>

struct PersonContextEpgMatch
{
    EpgPersonIndexMatch indexedMatch;
    std::string channelName;
};

struct PersonContextResult
{
    std::string name;
    std::string normalizedName;
    std::string providerPersonId;
    std::string identityMatch = "name-only";
    double confidence = 0.65;
    bool ambiguous = true;
    std::string imageBackendId;
    std::string imageChannelId;
    std::string imageEventId;
    int imageIndex = -1;
    RecordingPersonSearchResult recordings =
        RecordingPersonSearchResult::empty(50, 0);
    int epgTotalCount = 0;
    int epgLimit = 50;
    int epgOffset = 0;
    std::vector<PersonContextEpgMatch> epgMatches;
    bool externalEnabled = false;
};
