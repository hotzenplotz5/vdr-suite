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
    int creditCalls = 0;
    bool emptyCredits = false;
    std::string creditError;
    bool creditProviderAvailable = true;

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

    RecordingMetadataCastPage movieCredits(
        const std::string& movieExternalId,
        int limit) override
    {
        ++creditCalls;
        assert(movieExternalId == "13");
        assert(limit == 128);
        RecordingMetadataCastPage page;
        page.attempted = true;
        page.providerAvailable = creditProviderAvailable;
        page.providerId = "tmdb";
        page.error = creditError;
        if (!page.error.empty() || emptyCredits) return page;

        RecordingMetadataCastMember first;
        first.providerId = "tmdb";
        first.externalNamespace = "person";
        first.externalId = "31";
        first.name = "Tom Hanks";
        first.characterName = "Forrest Gump";
        first.order = 0;
        page.cast.push_back(first);

        RecordingMetadataCastMember second;
        second.providerId = "tmdb";
        second.externalNamespace = "person";
        second.externalId = "32";
        second.name = "Robin Wright";
        second.characterName = "Jenny Curran";
        second.order = 1;
        page.cast.push_back(second);
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

std::string assignmentBody(const std::string& resourceKey)
{
    return
        "{"
        "\"resourceKey\":\"" + resourceKey + "\","
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
        assert(provider.creditCalls == 0);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/seasons",
            "{\"seriesExternalId\":\"19885\",\"limit\":10}");
        assert(response.statusCode == 200);
        assert(response.body.find("Staffel 1") != std::string::npos);
        assert(provider.lastSeriesId == "19885");
        assert(provider.creditCalls == 0);
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
        assert(provider.creditCalls == 0);
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=missing",
            response));
        assert(response.statusCode == 200);
        assert(response.body == "{\"found\":false}");
        assert(provider.creditCalls == 0);
    }

    provider.creditError = "provider temporarily unavailable";
    provider.creditProviderAvailable = false;
    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody("failed/key"),
            "user:real-admin");
        assert(response.statusCode == 503);
        assert(response.body.find("metadata_cast_enrichment_failed") !=
            std::string::npos);
        assert(response.body.find("test-token") == std::string::npos);
        assert(provider.creditCalls == 1);

        ApiResponse readback;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=failed%2Fkey",
            readback));
        assert(readback.body == "{\"found\":false}");
        assert(provider.creditCalls == 1);
    }

    provider.creditError.clear();
    provider.creditProviderAvailable = true;
    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody("cache/key"),
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find("\"manual\":true") != std::string::npos);
        assert(response.body.find("\"revision\":1") != std::string::npos);
        assert(response.body.find("\"castComplete\":true") != std::string::npos);
        assert(response.body.find("Tom Hanks") != std::string::npos);
        assert(response.body.find("Forrest Gump") != std::string::npos);
        assert(response.body.find("Robin Wright") != std::string::npos);
        assert(response.body.find("Jenny Curran") != std::string::npos);
        assert(response.body.find("user:real-admin") == std::string::npos);
        assert(response.body.find("/candidate.jpg") == std::string::npos);
        assert(provider.creditCalls == 2);
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=cache%2Fkey",
            response));
        assert(response.statusCode == 200);
        assert(response.body.find("\"found\":true") != std::string::npos);
        assert(response.body.find("Tom Hanks") != std::string::npos);
        assert(response.body.find("\"characterName\":\"Forrest Gump\"") !=
            std::string::npos);
        assert(provider.creditCalls == 2);
    }

    provider.emptyCredits = true;
    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody("empty/key"),
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find("\"castComplete\":true") != std::string::npos);
        assert(response.body.find("\"people\":[]") != std::string::npos);
        assert(provider.creditCalls == 3);
    }
    provider.emptyCredits = false;

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody("cache/key"),
            "");
        assert(response.statusCode == 401);
        assert(provider.creditCalls == 3);
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
        assert(provider.creditCalls == 3);
    }

    {
        ApiResponse response;
        assert(!runtime.tryHandlePost(
            "/api/backends/living%2Froom/recordings/metadata/assign",
            assignmentBody("cache/key"),
            "user:test-admin",
            response));
        assert(!runtime.tryHandlePost(
            "/api/backends/living-room/recordings/metadata/manual",
            assignmentBody("cache/key"),
            "user:test-admin",
            response));
    }

    runtime.reset();
    database.close();
    return 0;
}
