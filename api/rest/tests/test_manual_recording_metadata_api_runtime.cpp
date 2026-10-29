#include "Database.h"
#include "ManualRecordingMetadataApiRuntime.h"
#include "MetadataController.h"
#include "MetadataRepository.h"
#include "MediaImport.h"
#include "RecordingMetadataCandidateProvider.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstdlib>
#include <unistd.h>

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
    int trailerCalls = 0;
    std::string lastTrailerMediaType;
    std::string lastTrailerExternalId;

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

    RecordingMetadataTrailerPage trailers(
        const std::string& mediaType,
        const std::string& externalId,
        int limit) override
    {
        ++trailerCalls;
        lastTrailerMediaType = mediaType;
        lastTrailerExternalId = externalId;
        assert(limit == 8);

        RecordingMetadataTrailerPage page;
        page.attempted = true;
        page.providerAvailable = true;
        page.providerId = "tmdb";

        RecordingMetadataTrailer trailer;
        trailer.providerId = "youtube";
        trailer.externalId = "abcdefghijk";
        trailer.title = "Offizieller Trailer";
        trailer.language = "de";
        trailer.official = true;
        page.trailers.push_back(trailer);
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
    assert(database.execute(
        "CREATE TABLE vdr_recording_cache("
        "backend_id TEXT NOT NULL,"
        "cache_key TEXT NOT NULL,"
        "backend_native_id TEXT NOT NULL DEFAULT '',"
        "PRIMARY KEY(backend_id,cache_key));"
        "INSERT INTO vdr_recording_cache VALUES"
        "('living-room','genre/key','genre-native');"));
    MetadataRepository repository(database);
    FakeProvider provider;
    MetadataController controller(repository, &provider);

    // Real API-to-writer integration, using a disposable existing VDR
    // recording. No FFmpeg, reimport, live VDR or TMDb/network lookup.
    char nativeRootTemplate[] = "/tmp/vdr-suite-meta6-native-XXXXXX";
    const char* createdRoot = ::mkdtemp(nativeRootTemplate);
    assert(createdRoot != nullptr);
    const std::filesystem::path nativeRoot(createdRoot);
    const auto nativeRecording = nativeRoot / "Serien" / "Andor" /
        "2026-10-28.17.06.1-0.rec";
    std::filesystem::create_directories(nativeRecording);
    const std::string legacyInfo =
        "C Channel\nT DE - Star Wars Andor S01E01\n"
        "S Imported via VDR-Suite Media Tools\n"
        "E 123 456 789 4E 00\nF 25\n";
    std::ofstream(nativeRecording / "info", std::ios::binary) << legacyInfo;
    std::ofstream(nativeRecording / "index", std::ios::binary) << "index";
    std::ofstream(nativeRecording / "00001.ts", std::ios::binary) << "video-bytes";

    int nativeUpdateCalls = 0;
    controller.setNativeInfoUpdate(
        [&nativeUpdateCalls, &nativeRoot, &nativeRecording](
            const ManualRecordingMetadataSelection& selected,
            const ManualRecordingMetadataAssignment& assigned) {
            ++nativeUpdateCalls;
            assert(selected.backendId == "living-room");
            assert(assigned.found);
            if (selected.mediaType != "episode")
                return selected.title == "Forrest Gump";
            assert(selected.resourceKey == "episode/key");
            assert(assigned.resourceKey == "episode/key");
            assert(selected.seriesTitle == "Star Wars: Andor");
            vdrsuite::mediatools::ImportConfirmedMetadata confirmed;
            confirmed.mediaType = assigned.mediaType;
            confirmed.title = assigned.title;
            confirmed.seriesTitle = selected.seriesTitle;
            confirmed.overview = assigned.overview;
            confirmed.seasonNumber = assigned.seasonNumber;
            confirmed.episodeNumber = assigned.episodeNumber;
            return vdrsuite::mediatools::updateConfirmedNativeVdrInfo(
                nativeRoot, nativeRecording, confirmed);
        });

    ManualRecordingMetadataApiRuntime& runtime =
        ManualRecordingMetadataApiRuntime::instance();
    runtime.reset();
    runtime.registerController(controller);

    std::vector<std::string> presentationChanges;
    runtime.registerRecordingPresentationChangedCallback(
        [&presentationChanges](const std::string& backendId)
        {
            presentationChanges.push_back(backendId);
        });

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

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/trailer?mediaType=movie&externalId=13",
            response));
        assert(response.statusCode == 200);
        assert(response.body.find("\"available\":true") != std::string::npos);
        assert(response.body.find("\"provider\":\"youtube\"") != std::string::npos);
        assert(response.body.find("\"videoId\":\"abcdefghijk\"") != std::string::npos);
        assert(response.body.find("\"official\":true") != std::string::npos);
        assert(provider.trailerCalls == 1);
        assert(provider.lastTrailerMediaType == "movie");
        assert(provider.lastTrailerExternalId == "13");
        assert(provider.creditCalls == 0);
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/trailer?mediaType=episode&externalId=13",
            response));
        assert(response.statusCode == 400);
        assert(response.body.find("invalid_metadata_trailer_identity") !=
            std::string::npos);
        assert(provider.trailerCalls == 1);
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
        assert(presentationChanges.empty());
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
        assert(response.body.find("\"nativeInfoUpdated\":true") != std::string::npos);
        assert(nativeUpdateCalls == 1);
        assert(response.body.find("\"revision\":1") != std::string::npos);
        assert(response.body.find("\"castComplete\":true") != std::string::npos);
        assert(response.body.find("Tom Hanks") != std::string::npos);
        assert(response.body.find("Forrest Gump") != std::string::npos);
        assert(response.body.find("Robin Wright") != std::string::npos);
        assert(response.body.find("Jenny Curran") != std::string::npos);
        assert(response.body.find("user:real-admin") == std::string::npos);
        assert(response.body.find("/candidate.jpg") == std::string::npos);
        assert(provider.creditCalls == 2);
        assert(presentationChanges ==
            std::vector<std::string>{"living-room"});
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
        assert(presentationChanges ==
            (std::vector<std::string>{"living-room", "living-room"}));
    }
    provider.emptyCredits = false;

    {
        // Existing Andor S01E01: the authenticated existing assignment
        // route applies the already-found provider data to native info,
        // without an extra API, another import or a TMDb query.
        const int beforeCredits = provider.creditCalls;
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            R"({"resourceKey":"episode/key","providerId":"tmdb",)"
            R"("externalNamespace":"tv-episode","externalId":"221",)"
            R"("mediaType":"episode","title":"Kassa",)"
            R"("seriesTitle":"Star Wars: Andor",)"
            R"("overview":"Die Suche beginnt.","posterReference":"",)"
            R"("seasonNumber":1,"episodeNumber":1,"expectedRevision":0})",
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find("\"nativeInfoUpdated\":true") != std::string::npos);
        assert(nativeUpdateCalls == 3);
        assert(provider.creditCalls == beforeCredits);
        std::ifstream info(nativeRecording / "info", std::ios::binary);
        const std::string written((std::istreambuf_iterator<char>(info)),
                                  std::istreambuf_iterator<char>());
        assert(written ==
            "C Channel\nT Star Wars: Andor\nS Kassa\n"
            "E 123 456 789 4E 00\nF 25\nD Die Suche beginnt.\n");
        assert(std::filesystem::exists(nativeRoot / ".update"));
        std::ifstream ts(nativeRecording / "00001.ts", std::ios::binary);
        assert(std::string(std::istreambuf_iterator<char>(ts),
                           std::istreambuf_iterator<char>()) == "video-bytes");
        std::ifstream index(nativeRecording / "index", std::ios::binary);
        assert(std::string(std::istreambuf_iterator<char>(index),
                           std::istreambuf_iterator<char>()) == "index");
        ApiResponse persisted;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/manual?resourceKey=episode%2Fkey",
            persisted));
        assert(persisted.body.find("\"found\":true") != std::string::npos);
        assert(persisted.body.find("\"title\":\"Kassa\"") != std::string::npos);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/assign",
            assignmentBody("cache/key"),
            "");
        assert(response.statusCode == 401);
        assert(provider.creditCalls == 3);
        assert(presentationChanges.size() == 2);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/withdraw",
            "{\"resourceKey\":\"cache/key\",\"expectedRevision\":9}");
        assert(response.statusCode == 409);
        assert(presentationChanges.size() == 2);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/withdraw",
            "{\"resourceKey\":\"cache/key\",\"expectedRevision\":1}");
        assert(response.statusCode == 200);
        assert(response.body.find("\"relationshipLocked\":false") != std::string::npos);
        assert(presentationChanges ==
            (std::vector<std::string>{
                "living-room", "living-room", "living-room"}));
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

    presentationChanges.clear();
    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/genre",
            "{\"resourceKey\":\"genre-native\",\"genreId\":\"science-fiction\"}",
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find(
            "\"selectedGenreId\":\"science-fiction\"") !=
            std::string::npos);
        assert(response.body.find(
            "{\"id\":\"science-fiction\",\"label\":\"Science-Fiction\"}") !=
            std::string::npos);
        assert(presentationChanges ==
            std::vector<std::string>{"living-room"});
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/genre?resourceKey=genre-native",
            response));
        assert(response.statusCode == 200);
        assert(response.body.find(
            "\"selectedGenreId\":\"science-fiction\"") !=
            std::string::npos);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/genre",
            "{\"resourceKey\":\"genre-native\",\"genreId\":\"not-a-canonical-genre\"}",
            "user:real-admin");
        assert(response.statusCode == 400);
        assert(presentationChanges.size() == 1);
    }

    {
        const ApiResponse response = post(
            runtime,
            "/api/backends/living-room/recordings/metadata/genre",
            "{\"resourceKey\":\"genre-native\",\"genreId\":\"\"}",
            "user:real-admin");
        assert(response.statusCode == 200);
        assert(response.body.find(
            "\"selectedGenreId\":\"\"") !=
            std::string::npos);
        assert(presentationChanges ==
            (std::vector<std::string>{"living-room", "living-room"}));
    }

    {
        ApiResponse response;
        assert(runtime.tryHandleGet(
            "/api/backends/living-room/recordings/metadata/genre?resourceKey=genre-native",
            response));
        assert(response.statusCode == 200);
        assert(response.body.find(
            "\"selectedGenreId\":\"\"") !=
            std::string::npos);
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
        assert(!runtime.tryHandlePost(
            "/api/backends/living-room/recordings/metadata/trailer",
            "{}",
            "user:test-admin",
            response));
    }

    runtime.reset();
    std::filesystem::remove_all(nativeRoot);
    database.close();
    return 0;
}
