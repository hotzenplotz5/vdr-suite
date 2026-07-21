#pragma once

#include "Person.h"
#include "VdrEvent.h"

#include <string>

struct EpgPersonIndexEntry
{
    std::string backendId;
    std::string channelId;
    std::string eventId;
    std::string originalName;
    std::string normalizedName;
    PersonRole role = PersonRole::Unknown;
    std::string characterName;
    std::string providerPersonId;
    std::string identityKind = "name-only";
    double confidence = 0.65;
    int personImageIndex = -1;
    std::string metadataEvidenceId;
};

struct EpgPersonIndexMatch
{
    EpgPersonIndexEntry person;
    VdrEvent event;
};

struct EpgPersonIndexQuery
{
    std::string backendId;
    std::string normalizedName;
    std::string providerPersonId;
    std::string fromTime;
    int limit = 50;
    int offset = 0;
};
