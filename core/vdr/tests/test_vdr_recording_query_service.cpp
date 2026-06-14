#include "MockVdrAdapter.h"
#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>

int main()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrRecordingQueryService queryService(vdrService);

    VdrRecordingQueryResult allResult =
        queryService.queryRecordings(
            VdrRecordingQuery::all());

    assert(allResult.totalCount() == 2);
    assert(allResult.returnedCount() == 2);
    assert(allResult.offset() == 0);
    assert(allResult.limit() == 0);
    assert(allResult.recordings().at(0).title == "Tagesschau");
    assert(allResult.recordings().at(1).title == "Tatort");

    VdrRecordingQueryResult limitedResult =
        queryService.queryRecordings(
            VdrRecordingQuery::limited(
                1,
                1));

    assert(limitedResult.totalCount() == 2);
    assert(limitedResult.returnedCount() == 1);
    assert(limitedResult.offset() == 1);
    assert(limitedResult.limit() == 1);
    assert(limitedResult.recordings().at(0).title == "Tatort");

    VdrRecordingQueryResult titleResult =
        queryService.queryRecordings(
            VdrRecordingQuery::byTitle(
                "tator",
                10,
                0));

    assert(titleResult.totalCount() == 1);
    assert(titleResult.returnedCount() == 1);
    assert(titleResult.recordings().at(0).title == "Tatort");

    VdrRecordingQueryResult sortedAscendingResult =
        queryService.queryRecordings(
            VdrRecordingQuery::sorted(
                "",
                "",
                0,
                0,
                VdrRecordingSortField::Title,
                VdrRecordingSortOrder::Ascending));

    assert(sortedAscendingResult.totalCount() == 2);
    assert(sortedAscendingResult.recordings().at(0).title == "Tagesschau");
    assert(sortedAscendingResult.recordings().at(1).title == "Tatort");

    VdrRecordingQueryResult sortedDescendingResult =
        queryService.queryRecordings(
            VdrRecordingQuery::sorted(
                "",
                "",
                0,
                0,
                VdrRecordingSortField::Title,
                VdrRecordingSortOrder::Descending));

    assert(sortedDescendingResult.totalCount() == 2);
    assert(sortedDescendingResult.recordings().at(0).title == "Tatort");
    assert(sortedDescendingResult.recordings().at(1).title == "Tagesschau");

    VdrRecordingQueryResult emptyResult =
        queryService.queryRecordings(
            VdrRecordingQuery::byTitle(
                "not-found",
                10,
                0));

    assert(emptyResult.empty());
    assert(emptyResult.totalCount() == 0);
    assert(emptyResult.returnedCount() == 0);

    std::cout
        << "test_vdr_recording_query_service passed"
        << std::endl;

    return 0;
}
