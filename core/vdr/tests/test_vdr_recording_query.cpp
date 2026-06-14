#include "VdrRecordingQuery.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrRecordingQuery all =
        VdrRecordingQuery::all();

    assert(!all.hasTitleFilter());
    assert(all.titleFilter().empty());
    assert(!all.hasLimit());
    assert(all.limit() == 0);
    assert(all.offset() == 0);

    VdrRecordingQuery limited =
        VdrRecordingQuery::limited(
            50,
            100);

    assert(!limited.hasTitleFilter());
    assert(limited.hasLimit());
    assert(limited.limit() == 50);
    assert(limited.offset() == 100);

    VdrRecordingQuery byTitle =
        VdrRecordingQuery::byTitle(
            "Tatort",
            25,
            50);

    assert(byTitle.hasTitleFilter());
    assert(byTitle.titleFilter() == "Tatort");
    assert(byTitle.hasLimit());
    assert(byTitle.limit() == 25);
    assert(byTitle.offset() == 50);

    std::cout
        << "test_vdr_recording_query passed"
        << std::endl;

    return 0;
}
