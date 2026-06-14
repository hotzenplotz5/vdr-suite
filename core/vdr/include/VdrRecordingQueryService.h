#pragma once

#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"

class VdrService;

class VdrRecordingQueryService
{
public:
    explicit VdrRecordingQueryService(
        VdrService& vdrService);

    VdrRecordingQueryResult queryRecordings(
        const VdrRecordingQuery& query) const;

private:
    VdrService& vdrService_;
};
