#include "VdrRecordingQueryMatcher.h"

#include <cassert>
#include <iostream>

static VdrRecording makeRecording(
    const std::string& id,
    const std::string& title)
{
    VdrRecording recording;
    recording.id = id;
    recording.backendId = "default";
    recording.title = title;
    recording.path = "/Mock/" + title + ".rec";
    recording.startTime = "2026-06-01T20:00:00";
    recording.durationSeconds = 900;
    recording.sizeMb = 512;
    return recording;
}

int main()
{
    VdrRecordingQueryMatcher matcher;

    VdrRecording tatort =
        makeRecording(
            "1",
            "Tatort");

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::all()));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::byTitle(
            "Tatort",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::byTitle(
            "tator",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::byTitle(
            "TATORT",
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        VdrRecordingQuery::byTitle(
            "Tagesschau",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::filtered(
            "",
            "Tatort",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::filtered(
            "tat",
            "mock",
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        VdrRecordingQuery::filtered(
            "tat",
            "Tagesschau",
            10,
            0)));

    VdrRecordingQuery backendQuery = VdrRecordingQuery::all();
    backendQuery.setBackendFilter("default");

    assert(matcher.matches(
        tatort,
        backendQuery));

    VdrRecordingQuery otherBackendQuery = VdrRecordingQuery::all();
    otherBackendQuery.setBackendFilter("ferienhaus");

    assert(!matcher.matches(
        tatort,
        otherBackendQuery));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::ranged(
            "",
            "",
            "2026-01-01T00:00:00",
            "2026-12-31T23:59:59",
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        VdrRecordingQuery::ranged(
            "",
            "",
            "2027-01-01T00:00:00",
            "",
            10,
            0)));

    assert(matcher.matches(
        tatort,
        VdrRecordingQuery::durationRanged(
            "",
            "",
            "",
            "",
            900,
            7200,
            10,
            0)));

    assert(!matcher.matches(
        tatort,
        VdrRecordingQuery::durationRanged(
            "",
            "",
            "",
            "",
            7200,
            0,
            10,
            0)));

    VdrRecording movie = tatort;
    movie.metadata.provider.contentKind = VdrRecordingContentKind::Movie;
    VdrRecordingQuery movies = VdrRecordingQuery::all();
    movies.setMovieReleaseYears(2022, 2026);
    for (const std::string date : {"2022", "2026-12-31", "2024-02-29", " 2023 "})
    {
        movie.metadata.provider.releaseDate = date;
        assert(matcher.matches(movie, movies));
    }
    for (const std::string date : {"2021", "2027", "", "2023-02-29", "2024-13-01", "2024-04-31", "2024-1-1"})
    {
        movie.metadata.provider.releaseDate = date;
        assert(!matcher.matches(movie, movies));
    }
    movie.metadata.provider.releaseDate = "2024";
    movie.metadata.provider.contentKind = VdrRecordingContentKind::SeriesEpisode;
    assert(!matcher.matches(movie, movies));
    assert(matcher.matches(movie, VdrRecordingQuery::all()));

    std::cout
        << "test_vdr_recording_query_matcher passed"
        << std::endl;

    return 0;
}
