#include "MockVdrAdapter.h"
#include "VdrRecordingQuery.h"
#include "VdrRecordingQueryResult.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

class DurationMockVdrAdapter : public MockVdrAdapter
{
public:
    explicit DurationMockVdrAdapter(VdrRecording recording)
        : recording_(std::move(recording))
    {
    }

    std::vector<VdrRecording> getRecordings() const override
    {
        return {recording_};
    }

private:
    VdrRecording recording_;
};

void writeSizedFile(
    const std::filesystem::path& path,
    std::uintmax_t size)
{
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    assert(stream.good());
    if (size > 0)
    {
        stream.seekp(static_cast<std::streamoff>(size - 1));
        stream.put('\0');
    }
    assert(stream.good());
}

void testVdrIndexDurationFallback()
{
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path() /
        "vdr-suite-recording-duration-query-test.rec";
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory);

    {
        std::ofstream info(directory / "info", std::ios::trunc);
        info << "F 50 1280 720 p 16:9\n";
    }

    constexpr int ExpectedDurationSeconds = 123;
    constexpr std::uintmax_t FramesPerSecond = 50;
    constexpr std::uintmax_t VdrIndexEntryBytes = 8;
    writeSizedFile(
        directory / "index",
        ExpectedDurationSeconds * FramesPerSecond * VdrIndexEntryBytes);

    VdrRecording recording;
    recording.id = "duration-fallback";
    recording.title = "Duration fallback";
    recording.path = directory.string();
    recording.backendNativeId = directory.string();
    recording.durationSeconds = 5400;
    recording.recordingDurationKnown = false;

    DurationMockVdrAdapter adapter(recording);
    VdrService vdrService(adapter);
    VdrRecordingQueryService queryService(vdrService);

    const VdrRecordingQueryResult catalog =
        queryService.queryRecordings(VdrRecordingQuery::all());
    assert(catalog.totalCount() == 1);
    assert(catalog.recordings().at(0).recordingDurationKnown);
    assert(catalog.recordings().at(0).durationSeconds == ExpectedDurationSeconds);

    VdrRecording resolved;
    assert(queryService.findRecordingById(
        "default",
        recording.id,
        resolved));
    assert(resolved.recordingDurationKnown);
    assert(resolved.durationSeconds == ExpectedDurationSeconds);

    {
        std::ofstream timer(directory / ".timer", std::ios::trunc);
        timer << "1@local\n";
    }

    const VdrRecordingQueryResult growingCatalog =
        queryService.queryRecordings(VdrRecordingQuery::all());
    assert(growingCatalog.totalCount() == 1);
    assert(!growingCatalog.recordings().at(0).recordingDurationKnown);
    assert(growingCatalog.recordings().at(0).durationSeconds == 0);

    VdrRecording growing;
    assert(queryService.findRecordingById(
        "default",
        recording.id,
        growing));
    assert(!growing.recordingDurationKnown);
    assert(growing.durationSeconds == 0);
    std::filesystem::remove(directory / ".timer", ignored);

    VdrRecording providerDuration = recording;
    providerDuration.recordingDurationKnown = true;
    providerDuration.durationSeconds = 77;
    DurationMockVdrAdapter providerAdapter(providerDuration);
    VdrService providerVdrService(providerAdapter);
    VdrRecordingQueryService providerQueryService(providerVdrService);

    const VdrRecordingQueryResult providerCatalog =
        providerQueryService.queryRecordings(VdrRecordingQuery::all());
    assert(providerCatalog.totalCount() == 1);
    assert(providerCatalog.recordings().at(0).recordingDurationKnown);
    assert(providerCatalog.recordings().at(0).durationSeconds == 77);

    VdrRecording providerResolved;
    assert(providerQueryService.findRecordingById(
        "default",
        providerDuration.id,
        providerResolved));
    assert(providerResolved.recordingDurationKnown);
    assert(providerResolved.durationSeconds == 77);

    std::filesystem::remove_all(directory, ignored);
}

} // namespace

int main()
{
    testVdrIndexDurationFallback();

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

    VdrRecording resolvedRecording;
    assert(queryService.findRecordingById(
        "default",
        allResult.recordings().at(1).id,
        resolvedRecording));
    assert(resolvedRecording.id == allResult.recordings().at(1).id);
    assert(resolvedRecording.title == "Tatort");
    assert(!resolvedRecording.path.empty());

    VdrRecording missingRecording;
    assert(!queryService.findRecordingById(
        "default",
        "recording-not-found",
        missingRecording));
    assert(!queryService.findRecordingById(
        "default",
        "",
        missingRecording));
    assert(!queryService.findRecordingById(
        "other-backend",
        allResult.recordings().at(1).id,
        missingRecording));

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

    VdrRecordingQueryResult sortedStartTimeDescendingResult =
        queryService.queryRecordings(
            VdrRecordingQuery::sorted(
                "",
                "",
                0,
                0,
                VdrRecordingSortField::StartTime,
                VdrRecordingSortOrder::Descending));

    assert(sortedStartTimeDescendingResult.totalCount() == 2);
    assert(sortedStartTimeDescendingResult.recordings().at(0).title == "Tatort");
    assert(sortedStartTimeDescendingResult.recordings().at(1).title == "Tagesschau");

    VdrRecordingQueryResult sortedDurationDescendingResult =
        queryService.queryRecordings(
            VdrRecordingQuery::sorted(
                "",
                "",
                0,
                0,
                VdrRecordingSortField::Duration,
                VdrRecordingSortOrder::Descending));

    assert(sortedDurationDescendingResult.totalCount() == 2);
    assert(sortedDurationDescendingResult.recordings().at(0).title == "Tatort");
    assert(sortedDurationDescendingResult.recordings().at(1).title == "Tagesschau");

    VdrRecordingQueryResult sortedSizeDescendingResult =
        queryService.queryRecordings(
            VdrRecordingQuery::sorted(
                "",
                "",
                0,
                0,
                VdrRecordingSortField::Size,
                VdrRecordingSortOrder::Descending));

    assert(sortedSizeDescendingResult.totalCount() == 2);
    assert(sortedSizeDescendingResult.recordings().at(0).title == "Tatort");
    assert(sortedSizeDescendingResult.recordings().at(1).title == "Tagesschau");

    VdrRecordingQueryResult rangedResult =
        queryService.queryRecordings(
            VdrRecordingQuery::ranged(
                "",
                "",
                "2026-06-01T20:00:00",
                "2026-06-01T21:00:00",
                10,
                0));

    assert(rangedResult.totalCount() == 2);
    assert(rangedResult.returnedCount() == 2);

    VdrRecordingQueryResult futureResult =
        queryService.queryRecordings(
            VdrRecordingQuery::ranged(
                "",
                "",
                "2027-01-01T00:00:00",
                "",
                10,
                0));

    assert(futureResult.empty());
    assert(futureResult.totalCount() == 0);

    VdrRecordingQueryResult durationResult =
        queryService.queryRecordings(
            VdrRecordingQuery::durationRanged(
                "",
                "",
                "",
                "",
                900,
                7200,
                10,
                0));

    assert(durationResult.totalCount() == 2);
    assert(durationResult.returnedCount() == 2);

    VdrRecordingQueryResult longDurationResult =
        queryService.queryRecordings(
            VdrRecordingQuery::durationRanged(
                "",
                "",
                "",
                "",
                7200,
                0,
                10,
                0));

    assert(longDurationResult.empty());
    assert(longDurationResult.totalCount() == 0);

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
