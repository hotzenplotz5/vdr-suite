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

    VdrRecordingQuery ranged =
        VdrRecordingQuery::ranged(
            "",
            "",
            "2026-01-01T00:00:00",
            "2026-12-31T23:59:59",
            20,
            2);

    assert(ranged.hasFromStartTime());
    assert(ranged.hasToStartTime());
    assert(ranged.fromStartTime() == "2026-01-01T00:00:00");
    assert(ranged.toStartTime() == "2026-12-31T23:59:59");
    assert(ranged.limit() == 20);
    assert(ranged.offset() == 2);

    VdrRecordingQuery durationRanged =
        VdrRecordingQuery::durationRanged(
            "",
            "",
            "",
            "",
            1800,
            7200,
            20,
            2);

    assert(durationRanged.hasMinDurationSeconds());
    assert(durationRanged.hasMaxDurationSeconds());
    assert(durationRanged.minDurationSeconds() == 1800);
    assert(durationRanged.maxDurationSeconds() == 7200);
    assert(durationRanged.limit() == 20);
    assert(durationRanged.offset() == 2);

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
    assert(VdrRecordingSortField::StartTime != VdrRecordingSortField::None);
    assert(VdrRecordingSortField::Duration != VdrRecordingSortField::None);
    assert(VdrRecordingSortField::Size != VdrRecordingSortField::None);
    assert(sorted.sortDescending());
    assert(sorted.limit() == 20);
    assert(sorted.offset() == 3);

    std::cout
        << "test_vdr_recording_query passed"
        << std::endl;

    return 0;
}
