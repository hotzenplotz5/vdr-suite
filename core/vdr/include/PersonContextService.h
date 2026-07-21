#pragma once

#include "EpgPersonIndexRepository.h"
#include "PersonContext.h"
#include "RecordingPersonSearchService.h"
#include "VdrSnapshotReadService.h"

#include <string>

class PersonContextService
{
public:
    PersonContextService(
        RecordingPersonSearchService& recordingPersonSearchService,
        EpgPersonIndexRepository& epgPersonIndexRepository,
        VdrSnapshotReadService& snapshotReadService);

    PersonContextResult getContext(
        const std::string& name,
        const std::string& providerPersonId,
        const std::string& backendId,
        const std::string& channelId,
        const std::string& eventId,
        const std::string& fromTime,
        int limit,
        int offset) const;

private:
    RecordingPersonSearchService& recordingPersonSearchService_;
    EpgPersonIndexRepository& epgPersonIndexRepository_;
    VdrSnapshotReadService& snapshotReadService_;
};
