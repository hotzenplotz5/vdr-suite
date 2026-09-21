#include "TmdbSeriesArtworkProvider.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
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

class FakeCache final : public ISeriesArtworkProviderCache
{
public:
    std::map<std::string, SeriesArtworkProviderCacheEntry> entries;
    int findCalls = 0;
    int storeCalls = 0;
    int removeCalls = 0;

    static std::string name(const SeriesArtworkProviderCacheKey& key)
    {
        return key.provider + ":" + key.identityProvider + ":" + key.identityValue;
    }

    SeriesArtworkProviderCacheEntry find(
        const SeriesArtworkProviderCacheKey& key,
        long long now) override
    {
        ++findCalls;
        const auto iterator = entries.find(name(key));
        if (iterator == entries.end() || !iterator->second.active(now))
        {
            return {};
        }
        return iterator->second;
    }

    bool store(
        const SeriesArtworkProviderCacheKey& key,
        SeriesArtworkProviderCacheOutcome outcome,
        long long expiresAt) override
    {
        ++storeCalls;
        entries[name(key)] = {outcome, expiresAt};
        return true;
    }

    bool remove(const SeriesArtworkProviderCacheKey& key) override
    {
        ++removeCalls;
        entries.erase(name(key));
        return true;
    }
};

ExternalArtworkHttpResponse jsonResponse(const std::string& body)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ExternalArtworkHttpResponse imageResponse(const std::string& body)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = "image/jpeg";
    response.body = body;
    return response;
}

EpgScraperMetadata metadata()
{
    EpgScraperMetadata value;
    value.backendId = "default";
    value.channelId = "channel";
    value.eventId = "event";
    value.provider = "tvscraper";
    value.mediaType = EpgScraperMediaType::Series;
    return value;
}

EpgScraperExternalId identity(
    EpgScraperExternalIdProvider provider,
    EpgScraperExternalIdScope scope,
    const std::string& value)
{
    EpgScraperExternalId result;
    result.provider = provider;
    result.scope = scope;
    result.value = value;
    return result;
}

TmdbSeriesArtworkProviderConfig config(const std::filesystem::path& incoming)
{
    TmdbSeriesArtworkProviderConfig value;
    value.readAccessToken = "test.token_value-123";
    value.incomingRoot = incoming.string();
    value.maximumRetries = 1;
    value.retryBackoffMs = 50;
    return value;
}

std::filesystem::path tempRoot(const std::string& name)
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-tmdb-provider-" + name);
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    return root;
}
}

int main()
{
    VdrEvent event{};
    event.id = "event";
    event.channelId = "channel";

    {
        const auto root = tempRoot("direct");
        FakeTransport transport;
        transport.responses = {
            jsonResponse(
                "{\"posters\":["
                "{\"file_path\":\"/neutral-poster.jpg\",\"width\":2000,\"height\":3000,\"vote_average\":9.0,\"iso_639_1\":null},"
                "{\"file_path\":\"/german-poster.jpg\",\"width\":1000,\"height\":1500,\"vote_average\":7.0,\"iso_639_1\":\"de\"}],"
                "\"backdrops\":["
                "{\"file_path\":\"/german-backdrop.jpg\",\"width\":3840,\"height\":2160,\"vote_average\":10.0,\"iso_639_1\":\"de\"}]}"),
            imageResponse("jpeg bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Imdb,
                     EpgScraperExternalIdScope::Series, "tt1234567"),
            identity(EpgScraperExternalIdProvider::Tmdb,
                     EpgScraperExternalIdScope::Series, "42")
        };
        std::vector<long long> sleeps;
        TmdbSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 1000LL; },
            [&](std::chrono::milliseconds delay) {
                sleeps.push_back(delay.count());
            });
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(result.artwork.provider == "tmdb");
        assert(result.artwork.width == 1000);
        assert(result.artwork.height == 1500);
        assert(std::filesystem::path(result.artwork.path).parent_path() == root);
        std::ifstream stored(result.artwork.path, std::ios::binary);
        std::string body((std::istreambuf_iterator<char>(stored)), {});
        assert(body == "jpeg bytes");
        assert(transport.requests.size() == 2);
        assert(transport.requests[0].url.find("/tv/42/images?") != std::string::npos);
        assert(transport.requests[0].bearerToken == "test.token_value-123");
        assert(transport.requests[1].url.find("/german-poster.jpg") != std::string::npos);
        assert(transport.requests[1].bearerToken.empty());
        assert(sleeps.empty());
        assert(cache.removeCalls == 1);
    }

    {
        const auto root = tempRoot("imdb");
        FakeTransport transport;
        transport.responses = {
            jsonResponse("{\"tv_results\":[{\"id\":77}]}"),
            jsonResponse("{\"backdrops\":[{\"file_path\":\"/series.png\",\"width\":1920,\"height\":1080,\"vote_average\":8,\"iso_639_1\":null}]}"),
            imageResponse("png bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Imdb,
                     EpgScraperExternalIdScope::Series, "tt7654321")
        };
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 2000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(result.artwork.width == 1920);
        assert(result.artwork.height == 1080);
        assert(transport.requests.size() == 3);
        assert(transport.requests[0].url.find("/find/tt7654321?") != std::string::npos);
        assert(transport.requests[0].url.find("external_source=imdb_id") != std::string::npos);
        assert(transport.requests[1].url.find("/tv/77/images?") != std::string::npos);
        assert(transport.requests[2].url.find("/series.png") != std::string::npos);
    }

    {
        const auto root = tempRoot("negative");
        FakeTransport transport;
        FakeCache cache;
        cache.entries["tmdb:tmdb:88"] = {
            SeriesArtworkProviderCacheOutcome::NotFound, 5000
        };
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Tmdb,
                     EpgScraperExternalIdScope::Series, "88")
        };
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 3000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted);
        assert(!result.found);
        assert(transport.requests.empty());
    }

    {
        const auto root = tempRoot("retry");
        FakeTransport transport;
        ExternalArtworkHttpResponse limited;
        limited.attempted = true;
        limited.statusCode = 429;
        limited.retryAfterSeconds = 1;
        transport.responses = {
            limited,
            jsonResponse("{\"backdrops\":[{\"file_path\":\"/retry.jpg\",\"width\":1920,\"height\":1080,\"iso_639_1\":null}]}"),
            imageResponse("retry bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Tmdb,
                     EpgScraperExternalIdScope::Series, "99")
        };
        std::vector<long long> sleeps;
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 4000LL; },
            [&](std::chrono::milliseconds delay) {
                sleeps.push_back(delay.count());
            });
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(sleeps.size() == 1);
        assert(sleeps.front() == 1000);
    }

    {
        const auto root = tempRoot("empty");
        FakeTransport transport;
        transport.responses = {
            jsonResponse("{\"posters\":[],\"backdrops\":[]}")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Tmdb,
                     EpgScraperExternalIdScope::Series, "101")
        };
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 5000LL; },
            [](std::chrono::milliseconds) {});
        const auto first = provider.resolve("default", event, value);
        assert(first.attempted && !first.found);
        assert(cache.storeCalls == 1);
        assert(cache.entries["tmdb:tmdb:101"].outcome ==
               SeriesArtworkProviderCacheOutcome::NotFound);
        const auto second = provider.resolve("default", event, value);
        assert(second.attempted && !second.found);
        assert(transport.requests.size() == 1);
    }

    {
        const auto root = tempRoot("ambiguous");
        FakeTransport transport;
        transport.responses = {
            jsonResponse("{\"tv_results\":[{\"id\":1},{\"id\":2}]}")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Tvdb,
                     EpgScraperExternalIdScope::Series, "1234")
        };
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 6000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted && !result.found);
        assert(cache.entries["tmdb:tvdb:1234"].outcome ==
               SeriesArtworkProviderCacheOutcome::NotFound);
        assert(transport.requests[0].url.find("external_source=tvdb_id") !=
               std::string::npos);
    }

    {
        const auto root = tempRoot("episode-scope");
        FakeTransport transport;
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Imdb,
                     EpgScraperExternalIdScope::Episode, "tt1234567")
        };
        TmdbSeriesArtworkProvider provider(
            transport, cache, config(root), [] { return 7000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(!result.attempted);
        assert(transport.requests.empty());
    }

    {
        TmdbSeriesArtworkProviderConfig invalid = config(tempRoot("invalid"));
        invalid.readAccessToken = "bad token";
        assert(!TmdbSeriesArtworkProvider::configurationValid(invalid));
        invalid = config(tempRoot("invalid2"));
        invalid.totalTimeoutMs = 100;
        assert(!TmdbSeriesArtworkProvider::configurationValid(invalid));
    }

    {
        const auto root = tempRoot("symlink-root");
        const auto realRoot = root / "real";
        const auto linkedRoot = root / "linked";
        std::filesystem::create_directory(realRoot);
        std::filesystem::create_directory_symlink(realRoot, linkedRoot);
        FakeTransport transport;
        transport.responses = {
            jsonResponse("{\"backdrops\":[{\"file_path\":\"/x.jpg\",\"width\":1920,\"height\":1080,\"iso_639_1\":null}]}"),
            imageResponse("bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(EpgScraperExternalIdProvider::Tmdb,
                     EpgScraperExternalIdScope::Series, "44")
        };
        auto providerConfig = config(linkedRoot);
        TmdbSeriesArtworkProvider provider(
            transport, cache, providerConfig, [] { return 8000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted && !result.found);
        assert(std::filesystem::is_empty(realRoot));
        assert(cache.entries["tmdb:tmdb:44"].outcome ==
               SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable);
    }

    return 0;
}
