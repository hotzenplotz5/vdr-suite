#include "Database.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "RecordingMetadataCandidateProvider.h"

#include <cassert>
#include <string>

namespace
{
class FakeProvider final : public IRecordingMetadataCandidateProvider
{
public:
    std::string lastQuery;
    std::string lastSeriesId;
    RecordingMetadataCandidateKind lastKind =
        RecordingMetadataCandidateKind::Movie;
    int lastSeason = 0;
    int lastLimit = 0;

    RecordingMetadataCandidatePage search(
        const std::string& query,
        RecordingMetadataCandidateKind kind,
        int limit) override
    {
        lastQuery = query;
        lastKind = kind;
        lastLimit = limit;
        RecordingMetadataCandidate candidate;
        candidate.kind = kind;
        candidate.providerId = "tmdb";
        candidate.externalNamespace =
            kind == RecordingMetadataCandidateKind::Movie ? "movie" : "tv";
        candidate.externalId =
            kind == RecordingMetadataCandidateKind::Movie ? "13" : "19885";
        candidate.title =
            kind == RecordingMetadataCandidateKind::Movie
                ? "Forrest Gump"
                : "Sherlock";
        candidate.originalTitle = candidate.title;
        candidate.overview = "Candidate overview";
        candidate.releaseDate = "1994-07-06";
        candidate.posterReference = "/candidate.jpg";

        RecordingMetadataCandidatePage page;
        page.attempted = true;
        page.providerAvailable = true;
        page.providerId = "tmdb";
        page.candidates.push_back(candidate);
        return page;
    }

    RecordingMetadataCandidatePage seasons(
        const std::string& seriesExternalId,
        int limit) override
    {
        lastSeriesId = seriesExternalId;
        lastLimit = limit;
        RecordingMetadataCandidate candidate;
        candidate.kind = RecordingMetadataCandidateKind::Season;
        candidate.providerId = "tmdb";
        candidate.externalNamespace = "tv-season";
        candidate.externalId = "3624";
        candidate.parentExternalId = seriesExternalId;
        candidate.title = "Staffel 1";
        candidate.posterReference = "/season.jpg";
        candidate.seasonNumber = 1;

        RecordingMetadataCandidatePage page;
        page.attempted = true;
        page.providerAvailable = true;
        page.providerId = "tmdb";
        page.candidates.push_back(candidate);
        return page;
    }

    RecordingMetadataCandidatePage episodes(
        const std::string& seriesExternalId,
        int seasonNumber,
        int limit) override
    {
        lastSeriesId = seriesExternalId;
        lastSeason = seasonNumber;
        lastLimit = limit;
        RecordingMetadataCandidate candidate;
        candidate.kind = RecordingMetadataCandidateKind::Episode;
        candidate.providerId = "tmdb";
        candidate.externalNamespace = "tv-episode";
        candidate.externalId = "123";
        candidate.parentExternalId = seriesExternalId;
        candidate.title = "Ein Fall von Pink";
        candidate.posterReference = "/episode.jpg";
        candidate.seasonNumber = seasonNumber;
        candidate.episodeNumber = 1;

        RecordingMetadataCandidatePage page;
        page.attempted = true;
        page.providerAvailable = true;
        page.providerId = "tmdb";
        page.candidates.push_back(candidate);
        return page;
    }
};

ApiResponse post(
    ManualRecordingMetadataApiRuntime& runtime,
    const std::string& route,
    const std::string& body,
    const std::string& actor = "user:test-admin")
{
    ApiResponse response;
    assert(runtime.tryHandlePost(route, body, actor, response));
    return response;
}
}

int main()
{
    Database database;
    assert(database.open(":memory:"));
    MetadataRepository repository(database);
    FakeProvider provider;
    MetadataController controller(repository, &provider);

    ManualRecordingMetadataApiRuntime& runtime =
        ManualRecordingMetadataApiRuntime::instance();
    runtime.reset();
    runtime.registerController(controller);

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/search",
            "{\"query\":\"Forrest Gump\",\"kind\":\"movie\",\"limit\":5}");
        assert(response.statusCode == 200);
        assert(response.body.find("Forrest Gump") != std::string::npos);
        assert(response.body.find("\"externalId\":\"13\"") != std::string::npos);
        assert(provider.lastQuery == "Forrest Gump");
        assert(provider.lastKind == RecordingMetadataCandidateKind::Movie);
        assert(provider.lastLimit == 5);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/seasons",
            "{\"seriesExternalId\":\"19885\",\"limit\":10}");
        assert(response.statusCode == 200);
        assert(response.body.find("Staffel 1") != std::string::npos);
        assert(provider.lastSeriesId == "19885");
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/episodes",
            "{\"seriesExternalId\":\"19885\",\"seasonNumber\":1,\"limit\":10}");
        assert(response.statusCode == 200);
        assert(response.body.find("Ein Fall von Pink") != std::string::npos);
        assert(provider.lastSeriesId == "19885");
        assert(provider.lastSeason == 1);
    }

    const std::string assignmentBody =
        "{"
        "\"resourceKey\":\"cache/key\","
        "\"providerId\":\"tmdb\","
        "\"externalNamespace\":\"movie\","
        "\"externalId\":\"13\","
        "\"mediaType\":\"movie\","
        "\"title\":\"Forrest Gump\","
        "\"originalTitle\":\"Forrest Gump\","
        "\"overview\":\"Selected candidate\","
        "\"releaseDate\":\"1994-07-06\","
        "\"posterReference\":\"/candidate.jpg\","
        "\"seasonNumber\":0,"
        "\"episodeNumber\":0,"
        "\"expectedRevision\":0"
        "}";

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody,
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find("\"manual\":true") != std::string::npos);
        assert(response.body.find("\"revision\":1") != std::string::npos);
        assert(response.body.find("user:real-admin") == std::string::npos);
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=cache%2Fkey",
            response));
        assert(response.statusCode == 200);
        assert(response.body.find("\"found\":true") != std::string::npos);
        assert(response.body.find("Forrest Gump") != std::string::npos);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody,
            "");
        assert(response.statusCode == 401);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/withdraw",
            "{\"resourceKey\":\"cache/key\",\"expectedRevision\":9}");
        assert(response.statusCode == 409);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/withdraw",
            "{\"resourceKey\":\"cache/key\",\"expectedRevision\":1}");
        assert(response.statusCode == 200);
        assert(response.body.find("\"relationshipLocked\":false") != std::string::npos);
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=cache%2Fkey",
            response));
        assert(response.statusCode == 200);
        assert(response.body == "{\"found\":false}");
    }

    {
        ApiResponse response;
        assert(!runtime.tryHandlePost(
            "/api/backends/living%2Froom/recordings/metadata/assign",
            assignmentBody,
            "user:test-admin",
            response));
        assert(!runtime.tryHandlePost(
            "/api/backends/living-room/recordings/metadata/manual",
            assignmentBody,
            "user:test-admin",
            response));
    }

    runtime.reset();
    database.close();
    return 0;
}
