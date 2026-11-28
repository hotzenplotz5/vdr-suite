#include "VdrRecordingQueryController.h"

#include "MockVdrAdapter.h"
#include "VdrRecordingQueryResultJsonSerializer.h"
#include "VdrRecordingQueryService.h"
#include "VdrService.h"

#include <cassert>
#include <iostream>
#include <map>
#include <string>

class RecentMoviesAdapter : public MockVdrAdapter
{
public:
    std::vector<VdrRecording> getRecordings() const override
    {
        std::vector<VdrRecording> recordings;
        for (int i = 0; i < 20; ++i)
        {
            VdrRecording recording;
            recording.id = "movie-" + std::to_string(i);
            recording.title = recording.id;
            recording.backendId = "default";
            recording.backendNativeId = "/Mock/" + recording.id + ".rec";
            recording.metadata.provider.contentKind = VdrRecordingContentKind::Movie;
            recording.metadata.provider.releaseDate = i < 18 ? "2000" : (i == 18 ? "2024" : "2025");
            recordings.push_back(recording);
        }
        return recordings;
    }
};

int main()
{
    MockVdrAdapter adapter;
    VdrService vdrService(adapter);
    VdrRecordingQueryService queryService(vdrService);
    VdrRecordingQueryResultJsonSerializer jsonSerializer;

    VdrRecordingQueryController controller(
        queryService,
        jsonSerializer);

    ApiResponse allResponse =
        controller.getRecordings();

    assert(allResponse.statusCode == 200);
    assert(allResponse.contentType == "application/json");
    assert(allResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(allResponse.body.find("\"returnedCount\":2") != std::string::npos);
    assert(allResponse.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(allResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);

    ApiResponse limitedResponse =
        controller.getRecordings(
            "",
            "",
            "",
            "",
            "",
            "",
            0,
            0,
            1,
            1);

    assert(limitedResponse.statusCode == 200);
    assert(limitedResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(limitedResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"limit\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"offset\":1") != std::string::npos);
    assert(limitedResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);

    ApiResponse titleResponse =
        controller.getRecordings(
            "tator",
            "",
            "",
            "",
            "",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(titleResponse.statusCode == 200);
    assert(titleResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(titleResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(titleResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);
    assert(titleResponse.body.find("\"title\":\"Tagesschau\"") == std::string::npos);

    ApiResponse backendResponse =
        controller.getRecordings(
            "",
            "default",
            "",
            "",
            "",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(backendResponse.statusCode == 200);
    assert(backendResponse.body.find("\"totalCount\":2") != std::string::npos);
    assert(backendResponse.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(backendResponse.body.find("\"title\":\"Tatort\"") != std::string::npos);

    ApiResponse pathResponse =
        controller.getRecordings(
            "",
            "Tagesschau",
            "",
            "",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(pathResponse.statusCode == 200);
    assert(pathResponse.body.find("\"totalCount\":1") != std::string::npos);
    assert(pathResponse.body.find("\"returnedCount\":1") != std::string::npos);
    assert(pathResponse.body.find("\"title\":\"Tagesschau\"") != std::string::npos);
    assert(pathResponse.body.find("\"title\":\"Tatort\"") == std::string::npos);

    ApiResponse sortedResponse =
        controller.getRecordings(
            "",
            "",
            "title",
            "desc",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(sortedResponse.statusCode == 200);
    assert(sortedResponse.body.find("\"title\":\"Tatort\"") <
           sortedResponse.body.find("\"title\":\"Tagesschau\""));

    ApiResponse startTimeSortResponse =
        controller.getRecordings(
            "",
            "",
            "startTime",
            "desc",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(startTimeSortResponse.statusCode == 200);
    assert(startTimeSortResponse.body.find("\"title\":\"Tatort\"") <
           startTimeSortResponse.body.find("\"title\":\"Tagesschau\""));

    ApiResponse rangeResponse =
        controller.getRecordings(
            "",
            "",
            "",
            "",
            "2026-06-01T20:00:00",
            "2026-06-01T21:00:00",
            0,
            0,
            10,
            0);

    assert(rangeResponse.statusCode == 200);
    assert(rangeResponse.body.find("\"totalCount\":2") != std::string::npos);

    ApiResponse durationResponse =
        controller.getRecordings(
            "",
            "",
            "",
            "",
            "",
            "",
            900,
            7200,
            10,
            0);

    assert(durationResponse.statusCode == 200);
    assert(durationResponse.body.find("\"totalCount\":2") != std::string::npos);

    int manualBatchCalls = 0;

    VdrRecordingQueryController manualController(
        queryService,
        jsonSerializer,
        [&manualBatchCalls](const std::string& backendId)
        {
            ++manualBatchCalls;
            assert(backendId == "default");

            ManualRecordingMetadataAssignment assignment;
            assignment.found = true;
            assignment.backendId = "default";
            assignment.resourceKey =
                "/Mock/Tatort/2026-06-01.20.15.1-0.rec";
            assignment.providerId = "tmdb";
            assignment.externalNamespace = "tmdb";
            assignment.externalId = "12345";
            assignment.mediaType = "movie";
            assignment.title = "Tatort manuell";
            assignment.originalTitle = "Tatort Manual";
            assignment.overview = "Manuell zugeordnete Metadaten";
            assignment.releaseDate = "2026-06-01";
            assignment.posterReference =
                "/var/cache/vdr-suite/recording-metadata/posters/tatort.webp";
            assignment.revision = 7;
            assignment.relationshipLocked = true;

            return std::map<
                std::string,
                ManualRecordingMetadataAssignment>{
                    {
                        assignment.resourceKey,
                        assignment
                    }
                };
        });

    const ApiResponse manualResponse =
        manualController.getRecordings(
            "",
            "default",
            "",
            "",
            "",
            "",
            "",
            0,
            0,
            10,
            0);

    assert(manualResponse.statusCode == 200);
    assert(manualBatchCalls == 1);
    assert(
        manualResponse.body.find(
            "\"source\":\"manual\"") !=
        std::string::npos);
    assert(
        manualResponse.body.find(
            "\"title\":\"Tatort manuell\"") !=
        std::string::npos);
    assert(
        manualResponse.body.find(
            "\"manualAssignment\":{\"active\":true,\"revision\":7") !=
        std::string::npos);
    assert(
        manualResponse.body.find(
            "assignmentRevision=7") !=
        std::string::npos);

    RecentMoviesAdapter moviesAdapter;
    VdrService moviesService(moviesAdapter);
    VdrRecordingQueryService moviesQuery(moviesService);
    VdrRecordingQueryController moviesController(moviesQuery, jsonSerializer);
    const auto moviesPage = moviesController.getRecordings(
        "", "", "", "", "", "", "", 0, 0, 1, 1, 2022, 2026);
    assert(moviesPage.body.find("\"totalCount\":2") != std::string::npos);
    assert(moviesPage.body.find("\"id\":\"movie-19\"") != std::string::npos);
    assert(moviesPage.body.find("\"id\":\"movie-0\"") == std::string::npos);
    const auto firstOrdered = moviesController.getRecordings(
        "", "", "", "movieReleaseDate", "desc", "", "", 0, 0, 1, 0, 2022, 2026);
    const auto nextOrdered = moviesController.getRecordings(
        "", "", "", "movieReleaseDate", "desc", "", "", 0, 0, 1, 1, 2022, 2026);
    assert(firstOrdered.body.find("\"totalCount\":2") != std::string::npos);
    assert(firstOrdered.body.find("\"id\":\"movie-19\"") != std::string::npos);
    assert(nextOrdered.body.find("\"id\":\"movie-18\"") != std::string::npos);
    VdrRecordingQueryController manualMovies(moviesQuery, jsonSerializer,
        [](const std::string&) {
            ManualRecordingMetadataAssignment assignment;
            assignment.found = true;
            assignment.relationshipLocked = true;
            assignment.mediaType = "movie";
            assignment.releaseDate = "2023";
            return std::map<std::string, ManualRecordingMetadataAssignment>{{"movie-19", assignment}};
        });
    const auto manualOrdered = manualMovies.getRecordings(
        "", "", "", "movieReleaseDate", "desc", "", "", 0, 0, 1, 0, 2022, 2026);
    assert(manualOrdered.body.find("\"id\":\"movie-18\"") != std::string::npos);
    const auto publicHome = moviesController.homePage("default", true, 2022, 2026, 1, 0);
    assert(publicHome.totalCount == 2 && publicHome.items.size() == 1U);
    assert(publicHome.items.front().recording.id == "movie-19");
    assert(publicHome.items.front().releaseDate == "2025");
    assert(!publicHome.items.front().manualAssignment.found);
    const auto manualHome = manualMovies.homePage("default", true, 2022, 2026, 1, 0);
    assert(manualHome.totalCount == 2 && manualHome.items.front().recording.id == "movie-18");
    const auto manualTail = manualMovies.homePage("default", true, 2022, 2026, 1, 1);
    assert(manualTail.totalCount == 2 && manualTail.items.size() == 1U);
    assert(manualTail.items.front().recording.id == "movie-19");
    assert(manualTail.items.front().releaseDate == "2023");
    assert(manualTail.items.front().manualAssignment.relationshipLocked);
    int identityLookups = 0;
    VdrRecordingQueryController publicMovies(moviesQuery, jsonSerializer, {},
        [&identityLookups](const std::string& backend, const std::string& native)
        {
            ++identityLookups;
            assert(backend == "default");
            assert(native == "/Mock/movie-19.rec");
            return std::string("rec_0123456789abcdef0123456789abcdef");
        });
    const auto publicPage = publicMovies.getRecordings(
        "", "default", "", "movieReleaseDate", "desc", "", "", 0, 0, 1, 0, 2022, 2026);
    assert(publicPage.statusCode == 200);
    assert(identityLookups == 1);
    assert(publicPage.body.find("\"publicRecordingId\":\"rec_0123456789abcdef0123456789abcdef\"") != std::string::npos);
    assert(publicPage.body.find("\"recordingId\":\"movie-19\"") != std::string::npos);
    assert(publicPage.body.find("\"totalCount\":2") != std::string::npos);

    const auto unchangedPage = moviesController.getRecordings();
    assert(unchangedPage.body.find("\"totalCount\":20") != std::string::npos);

    std::cout
        << "test_vdr_recording_query_controller passed"
        << std::endl;

    return 0;
}
