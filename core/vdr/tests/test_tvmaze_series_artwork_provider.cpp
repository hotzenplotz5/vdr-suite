#include "TvmazeSeriesArtworkJson.h"
#include "TvmazeSeriesArtworkProvider.h"

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
    int storeCalls = 0;
    int removeCalls = 0;

    static std::string name(const SeriesArtworkProviderCacheKey& key)
    {
        return key.provider + ":" + key.identityProvider + ":" +
            key.identityValue;
    }

    SeriesArtworkProviderCacheEntry find(
        const SeriesArtworkProviderCacheKey& key,
        long long now) override
    {
        const auto iterator = entries.find(name(key));
        if (iterator == entries.end() || !iterator->second.active(now))
            return {};
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

ExternalArtworkHttpResponse redirectResponse(
    const std::string& location,
    long statusCode = 301)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = statusCode;
    response.location = location;
    return response;
}

ExternalArtworkHttpResponse jsonResponse(const std::string& body)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = "application/json";
    response.body = body;
    return response;
}

ExternalArtworkHttpResponse imageResponse(
    const std::string& body,
    const std::string& contentType = "image/jpeg")
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = contentType;
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

TvmazeSeriesArtworkProviderConfig config(
    const std::filesystem::path& incoming)
{
    TvmazeSeriesArtworkProviderConfig value;
    value.incomingRoot = incoming.string();
    value.maximumRetries = 1;
    value.retryBackoffMs = 50;
    return value;
}

std::filesystem::path tempRoot(const std::string& name)
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        ("vdr-suite-tvmaze-provider-" + name);
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root);
    return root;
}

const std::string Images =
    "["
    "{\"id\":7,\"type\":\"poster\",\"main\":true,"
    "\"resolutions\":{\"original\":{"
    "\"url\":\"https://static.tvmaze.com/uploads/images/original_untouched/1/7.jpg\","
    "\"width\":1000,\"height\":1500}}},"
    "{\"id\":8,\"type\":\"background\",\"main\":false,"
    "\"resolutions\":{\"original\":{"
    "\"url\":\"https://static.tvmaze.com/uploads/images/original_untouched/2/8.jpg\","
    "\"width\":1280,\"height\":720}}},"
    "{\"id\":9,\"type\":\"background\",\"main\":false,"
    "\"resolutions\":{\"original\":{"
    "\"url\":\"https://static.tvmaze.com/uploads/images/original_untouched/2/9.jpg\","
    "\"width\":1920,\"height\":1080}}}"
    "]";
}

int main()
{
    {
        int id = 0;
        assert(parseTvmazeShowLocation("/shows/42", id) && id == 42);
        assert(parseTvmazeShowLocation(
            "https://api.tvmaze.com/shows/77", id) && id == 77);
        assert(parseTvmazeShowLocation(
            "http://api.tvmaze.com/shows/88", id) && id == 88);
        assert(!parseTvmazeShowLocation(
            "https://evil.example/shows/1", id));
        assert(!parseTvmazeShowLocation("/shows/01", id));
        assert(!parseTvmazeShowLocation("/shows/1/episodes", id));

        TvmazeSeriesImage image;
        assert(parseTvmazeSeriesImage(Images, 1024U * 1024U, image));
        assert(image.imageId == 9);
        assert(image.background);
        assert(image.width == 1920);
        assert(image.height == 1080);
    }

    VdrEvent event;
    event.id = "event";
    event.channelId = "channel";

    {
        const auto root = tempRoot("imdb");
        FakeTransport transport;
        transport.responses = {
            redirectResponse("/shows/77"),
            jsonResponse(Images),
            imageResponse("jpeg bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Tvdb,
                EpgScraperExternalIdScope::Series,
                "81189"),
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Series,
                "tt0944947")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 1000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);

        assert(result.valid());
        assert(result.artwork.provider == "tvmaze");
        assert(result.artwork.width == 1920);
        assert(result.artwork.height == 1080);
        assert(std::filesystem::path(result.artwork.path).parent_path() == root);
        assert(std::filesystem::path(result.artwork.path)
                   .filename().string().find("tvmaze-77-") == 0U);

        std::ifstream stored(result.artwork.path, std::ios::binary);
        const std::string body(
            (std::istreambuf_iterator<char>(stored)),
            std::istreambuf_iterator<char>());
        assert(body == "jpeg bytes");

        assert(transport.requests.size() == 3U);
        assert(transport.requests[0].url ==
               "https://api.tvmaze.com/lookup/shows?imdb=tt0944947");
        assert(transport.requests[0].bearerToken.empty());
        assert(transport.requests[1].url ==
               "https://api.tvmaze.com/shows/77/images");
        assert(transport.requests[2].url ==
               "https://static.tvmaze.com/uploads/images/original_untouched/2/9.jpg");
        assert(cache.removeCalls == 1);
    }

    {
        const auto root = tempRoot("tvdb");
        FakeTransport transport;
        transport.responses = {
            redirectResponse("https://api.tvmaze.com/shows/55"),
            jsonResponse(Images),
            imageResponse("png bytes", "image/png")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Tvdb,
                EpgScraperExternalIdScope::Series,
                "1234")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 2000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(transport.requests.front().url ==
               "https://api.tvmaze.com/lookup/shows?thetvdb=1234");
    }

    {
        const auto root = tempRoot("signed-tvdb");
        FakeTransport transport;
        transport.responses = {
            redirectResponse("/shows/56", 302),
            jsonResponse(Images),
            imageResponse("signed tvdb bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.providerId = -1234;
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Series,
                "tt7654321")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 2500LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(transport.requests.front().url ==
               "https://api.tvmaze.com/lookup/shows?thetvdb=1234");
        assert(std::filesystem::path(result.artwork.path)
                   .filename().string().find("tvmaze-56-") == 0U);
    }

    {
        const auto root = tempRoot("negative-cache");
        FakeTransport transport;
        FakeCache cache;
        cache.entries["tvmaze:imdb:tt1234567"] = {
            SeriesArtworkProviderCacheOutcome::NotFound,
            5000
        };
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Series,
                "tt1234567")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 3000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted && !result.found);
        assert(transport.requests.empty());
    }

    {
        const auto root = tempRoot("not-found");
        FakeTransport transport;
        ExternalArtworkHttpResponse notFound;
        notFound.attempted = true;
        notFound.statusCode = 404;
        transport.responses = {notFound};
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Series,
                "tt7654321")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 4000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted && !result.found);
        assert(cache.entries["tvmaze:imdb:tt7654321"].outcome ==
               SeriesArtworkProviderCacheOutcome::NotFound);
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
            redirectResponse("/shows/99"),
            jsonResponse(Images),
            imageResponse("retry bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Tvdb,
                EpgScraperExternalIdScope::Series,
                "999")
        };
        std::vector<long long> sleeps;

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 5000LL; },
            [&](std::chrono::milliseconds delay) {
                sleeps.push_back(delay.count());
            });
        const auto result = provider.resolve("default", event, value);
        assert(result.valid());
        assert(sleeps.size() == 1U);
        assert(sleeps.front() == 1000);
    }

    {
        const auto root = tempRoot("scope");
        FakeTransport transport;
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.providerId = 42;
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Episode,
                "tt1234567"),
            identity(
                EpgScraperExternalIdProvider::Tmdb,
                EpgScraperExternalIdScope::Series,
                "42")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(root),
            [] { return 6000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(!result.attempted);
        assert(transport.requests.empty());
    }

    {
        TvmazeSeriesArtworkProviderConfig invalid =
            config(tempRoot("invalid"));
        invalid.totalTimeoutMs = 100;
        assert(!TvmazeSeriesArtworkProvider::configurationValid(invalid));
    }

    {
        const auto root = tempRoot("symlink-root");
        const auto realRoot = root / "real";
        const auto linkedRoot = root / "linked";
        std::filesystem::create_directory(realRoot);
        std::filesystem::create_directory_symlink(realRoot, linkedRoot);

        FakeTransport transport;
        transport.responses = {
            redirectResponse("/shows/44"),
            jsonResponse(Images),
            imageResponse("bytes")
        };
        FakeCache cache;
        EpgScraperMetadata value = metadata();
        value.externalIds = {
            identity(
                EpgScraperExternalIdProvider::Imdb,
                EpgScraperExternalIdScope::Series,
                "tt1234567")
        };

        TvmazeSeriesArtworkProvider provider(
            transport,
            cache,
            config(linkedRoot),
            [] { return 7000LL; },
            [](std::chrono::milliseconds) {});
        const auto result = provider.resolve("default", event, value);
        assert(result.attempted && !result.found);
        assert(std::filesystem::is_empty(realRoot));
        assert(cache.entries["tvmaze:imdb:tt1234567"].outcome ==
               SeriesArtworkProviderCacheOutcome::TemporarilyUnavailable);
    }

    return 0;
}
