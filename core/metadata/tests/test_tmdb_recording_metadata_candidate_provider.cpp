#include "IExternalArtworkHttpTransport.h"
#include "TmdbRecordingMetadataCandidateProvider.h"

#include <cassert>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace
{
class FakeTransport final : public IExternalArtworkHttpTransport
{
public:
    std::vector<ExternalArtworkHttpRequest> requests;
    std::vector<ExternalArtworkHttpResponse> responses;

    ExternalArtworkHttpResponse perform(
        const ExternalArtworkHttpRequest& request) override
    {
        requests.push_back(request);
        assert(!responses.empty());
        ExternalArtworkHttpResponse response = responses.front();
        responses.erase(responses.begin());
        return response;
    }
};

ExternalArtworkHttpResponse jsonResponse(const std::string& body)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = "application/json; charset=utf-8";
    response.body = body;
    return response;
}

TmdbRecordingMetadataCandidateProviderConfig config()
{
    TmdbRecordingMetadataCandidateProviderConfig result;
    result.readAccessToken = "test-token";
    result.language = "de-DE";
    result.maximumRetries = 1;
    result.retryBackoffMs = 50;
    return result;
}
}

int main()
{
    {
        FakeTransport transport;
        transport.responses.push_back(jsonResponse(R"json({
          "results": [
            {
              "id": 13,
              "title": "Forrest Gump",
              "original_title": "Forrest Gump",
              "overview": "Ein Film.",
              "release_date": "1994-07-06",
              "poster_path": "/forrest.jpg",
              "vote_average": 8.5
            },
            {
              "id": 999,
              "title": "Forrest",
              "original_title": "Forrest",
              "overview": "Zweiter Treffer.",
              "release_date": "2020-01-01",
              "poster_path": "/forrest-2.jpg",
              "vote_average": 6.0
            }
          ]
        })json"));
        TmdbRecordingMetadataCandidateProvider provider(
            transport, config(), [](std::chrono::milliseconds) {});
        const auto page = provider.search(
            "Forrest Gump",
            RecordingMetadataCandidateKind::Movie,
            1);
        assert(page.attempted);
        assert(page.providerAvailable);
        assert(page.error.empty());
        assert(page.truncated);
        assert(page.candidates.size() == 1U);
        const auto& candidate = page.candidates.front();
        assert(candidate.valid());
        assert(candidate.kind == RecordingMetadataCandidateKind::Movie);
        assert(candidate.providerId == "tmdb");
        assert(candidate.externalNamespace == "movie");
        assert(candidate.externalId == "13");
        assert(candidate.title == "Forrest Gump");
        assert(candidate.posterReference == "/forrest.jpg");
        assert(transport.requests.size() == 1U);
        assert(transport.requests.front().url.find(
            "/search/movie?query=Forrest%20Gump") != std::string::npos);
        assert(transport.requests.front().bearerToken == "test-token");
    }

    {
        FakeTransport transport;
        transport.responses.push_back(jsonResponse(R"json({
          "results": [
            {
              "id": 19885,
              "name": "Sherlock",
              "original_name": "Sherlock",
              "overview": "Krimiserie.",
              "first_air_date": "2010-07-25",
              "poster_path": "/sherlock.jpg",
              "vote_average": 8.5
            }
          ]
        })json"));
        TmdbRecordingMetadataCandidateProvider provider(transport, config());
        const auto page = provider.search(
            "Sherlock",
            RecordingMetadataCandidateKind::Series,
            10);
        assert(page.providerAvailable);
        assert(page.candidates.size() == 1U);
        assert(page.candidates.front().kind == RecordingMetadataCandidateKind::Series);
        assert(page.candidates.front().externalNamespace == "tv");
        assert(page.candidates.front().externalId == "19885");
        assert(transport.requests.front().url.find("/search/tv?") != std::string::npos);
    }

    {
        FakeTransport transport;
        transport.responses.push_back(jsonResponse(R"json({
          "seasons": [
            {
              "id": 3624,
              "name": "Staffel 1",
              "overview": "Erste Staffel.",
              "air_date": "2010-07-25",
              "poster_path": "/season-1.jpg",
              "season_number": 1
            },
            {
              "id": 0,
              "name": "Specials",
              "season_number": 0
            }
          ]
        })json"));
        TmdbRecordingMetadataCandidateProvider provider(transport, config());
        const auto page = provider.seasons("19885", 10);
        assert(page.providerAvailable);
        assert(page.candidates.size() == 1U);
        const auto& season = page.candidates.front();
        assert(season.kind == RecordingMetadataCandidateKind::Season);
        assert(season.externalNamespace == "tv-season");
        assert(season.externalId == "3624");
        assert(season.parentExternalId == "19885");
        assert(season.seasonNumber == 1);
        assert(transport.requests.front().url.find("/tv/19885?") != std::string::npos);
    }

    {
        FakeTransport transport;
        transport.responses.push_back(jsonResponse(R"json({
          "episodes": [
            {
              "id": 123,
              "name": "Ein Fall von Pink",
              "overview": "Erste Folge.",
              "air_date": "2010-07-25",
              "still_path": "/episode-1.jpg",
              "season_number": 1,
              "episode_number": 1,
              "vote_average": 8.1
            }
          ]
        })json"));
        TmdbRecordingMetadataCandidateProvider provider(transport, config());
        const auto page = provider.episodes("19885", 1, 10);
        assert(page.providerAvailable);
        assert(page.candidates.size() == 1U);
        const auto& episode = page.candidates.front();
        assert(episode.kind == RecordingMetadataCandidateKind::Episode);
        assert(episode.externalNamespace == "tv-episode");
        assert(episode.externalId == "123");
        assert(episode.parentExternalId == "19885");
        assert(episode.seasonNumber == 1);
        assert(episode.episodeNumber == 1);
        assert(transport.requests.front().url.find(
            "/tv/19885/season/1?") != std::string::npos);
    }

    {
        FakeTransport transport;
        ExternalArtworkHttpResponse rateLimited;
        rateLimited.attempted = true;
        rateLimited.statusCode = 429;
        rateLimited.retryAfterSeconds = 1;
        transport.responses.push_back(rateLimited);
        transport.responses.push_back(jsonResponse("{\"results\":[]}"));
        int sleeps = 0;
        TmdbRecordingMetadataCandidateProvider provider(
            transport,
            config(),
            [&](std::chrono::milliseconds delay) {
                ++sleeps;
                assert(delay.count() == 1000);
            });
        const auto page = provider.search(
            "Matrix",
            RecordingMetadataCandidateKind::Movie,
            10);
        assert(page.providerAvailable);
        assert(page.error.empty());
        assert(page.candidates.empty());
        assert(transport.requests.size() == 2U);
        assert(sleeps == 1);
    }

    {
        FakeTransport transport;
        TmdbRecordingMetadataCandidateProvider provider(transport, config());
        const auto invalid = provider.search(
            "x",
            RecordingMetadataCandidateKind::Episode,
            100);
        assert(!invalid.attempted);
        assert(!invalid.error.empty());
        assert(transport.requests.empty());
    }

    return 0;
}
