#include "VdrRecordingQuery.h"

#include <cassert>
#include <iostream>

int main()
{
    VdrRecordingQuery all =
        VdrRecordingQuery::all();

    assert(!all.hasTitleFilter());
    assert(all.titleFilter().empty());
    assert(!all.hasPathFilter());
    assert(all.pathFilter().empty());
    assert(!all.hasLimit());
    assert(all.limit() == 0);
    assert(all.offset() == 0);

    VdrRecordingQuery limited =
        VdrRecordingQuery::limited(
            50,
            100);

    assert(!limited.hasTitleFilter());
    assert(!limited.hasPathFilter());
    assert(limited.pathFilter().empty());
    assert(limited.hasLimit());
    assert(limited.limit() == 50);
    assert(limited.offset() == 100);

    VdrRecordingQuery byTitle =
        VdrRecordingQuery::byTitle(
            "Tatort",
            25,
            50);

    assert(byTitle.hasTitleFilter());
    assert(!byTitle.hasPathFilter());
    assert(byTitle.titleFilter() == "Tatort");
    assert(byTitle.pathFilter().empty());
    assert(byTitle.hasLimit());
    assert(byTitle.limit() == 25);
    assert(byTitle.offset() == 50);

    VdrRecordingQuery filtered =
        VdrRecordingQuery::filtered(
            "Tatort",
            "Krimi",
            10,
            5);

    assert(filtered.hasTitleFilter());
    assert(filtered.hasPathFilter());
    assert(filtered.titleFilter() == "Tatort");
    assert(filtered.pathFilter() == "Krimi");
    assert(filtered.limit() == 10);
    assert(filtered.offset() == 5);

    VdrRecordingQuery sorted =
        VdrRecordingQuery::sorted(
            "",
            "",
            20,
            3,
            VdrRecordingSortField::Title,
            VdrRecordingSortOrder::Descending);

    assert(sorted.hasSort());
    assert(sorted.sortField() == VdrRecordingSortField::Title);
    assert(sorted.sortOrder() == VdrRecordingSortOrder::Descending);
    assert(sorted.sortDescending());
    assert(sorted.limit() == 20);
    assert(sorted.offset() == 3);

    std::cout
        << "test_vdr_recording_query passed"
        << std::endl;

    return 0;
}
