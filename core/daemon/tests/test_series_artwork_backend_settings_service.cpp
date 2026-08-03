#include "SeriesArtworkBackendSettingsService.h"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <map>
#include <sqlite3.h>
#include <string>
#include <sys/stat.h>
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
        entries[name(key)] = {outcome, expiresAt};
        return true;
    }

    bool remove(const SeriesArtworkProviderCacheKey& key) override
    {
        entries.erase(name(key));
        return true;
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

ExternalArtworkHttpResponse imageResponse(const std::string& body)
{
    ExternalArtworkHttpResponse response;
    response.attempted = true;
    response.statusCode = 200;
    response.contentType = "image/jpeg";
    response.body = body;
    return response;
}

std::filesystem::path tempRoot()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-series-artwork-backend-settings-test";
    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root / "secrets");
    std::filesystem::create_directories(root / "incoming");
    std::filesystem::permissions(
        root / "secrets",
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace);
    return root;
}

mode_t fileMode(const std::filesystem::path& path)
{
    struct stat metadata{};
    assert(::stat(path.c_str(), &metadata) == 0);
    return metadata.st_mode & 0777;
}

int scalar(Database& database, const std::string& sql)
{
    sqlite3_stmt* statement = nullptr;
    assert(sqlite3_prepare_v2(
        database.handle(), sql.c_str(), -1, &statement, nullptr) == SQLITE_OK);
    assert(sqlite3_step(statement) == SQLITE_ROW);
    const int value = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return value;
}
}

int main()
{
    const std::filesystem::path root = tempRoot();

    Database database;
    assert(database.open((root / "settings.db").string()));

    assert(database.execute(
        "CREATE TABLE epg_scraper_metadata_cache ("
        "backend_id TEXT NOT NULL,channel_id TEXT NOT NULL,event_id TEXT NOT NULL,"
        "public_json TEXT NOT NULL,resolved_at INTEGER NOT NULL DEFAULT 0,"
        "updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "PRIMARY KEY(backend_id,channel_id,event_id));"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'default','channel','missing-series',"
        "'{\"mediaType\":\"series\",\"preferredArtwork\":{\"available\":false}}',"
        "1,CURRENT_TIMESTAMP);"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'default','channel','pictured-series',"
        "'{\"mediaType\":\"series\",\"preferredArtwork\":{\"available\":true}}',"
        "1,CURRENT_TIMESTAMP);"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'default','channel','missing-movie',"
        "'{\"mediaType\":\"movie\",\"preferredArtwork\":{\"available\":false}}',"
        "1,CURRENT_TIMESTAMP);"
        "INSERT INTO epg_scraper_metadata_cache VALUES("
        "'house-b','channel','missing-series',"
        "'{\"mediaType\":\"series\",\"preferredArtwork\":{\"available\":false}}',"
        "1,CURRENT_TIMESTAMP);"));

    FakeTransport transport;
    FakeCache cache;

    SeriesArtworkBackendSettingsConfig config;
    config.defaultProvider = "tvmaze";
    config.secretRoot = (root / "secrets").string();
    config.tmdb.incomingRoot = (root / "incoming").string();
    config.tvmaze.incomingRoot = (root / "incoming").string();

    SeriesArtworkBackendSettingsService service(
        database,
        transport,
        cache,
        config);
    assert(service.ensureSchema());

    const SeriesArtworkBackendSettingsSnapshot initial =
        service.get("default");
    assert(initial.backendId == "default");
    assert(initial.provider == "tvmaze");
    assert(initial.configurationSource == "environment");
    assert(!initial.tmdbTokenConfigured);

    transport.responses = {jsonResponse("{}")};
    SeriesArtworkBackendSettingsUpdate enableTmdb;
    enableTmdb.backendId = "default";
    enableTmdb.provider = "tmdb";
    enableTmdb.tmdbReadAccessToken = "test.token_value-123";

    const SeriesArtworkBackendSettingsUpdateResult enabled =
        service.update(enableTmdb);
    assert(enabled.success);
    assert(enabled.statusCode == 200);
    assert(enabled.settings.provider == "tmdb");
    assert(enabled.settings.configurationSource == "managed");
    assert(enabled.settings.tmdbTokenConfigured);
    assert(enabled.settings.tmdbTokenSource == "managed");
    assert(transport.requests.size() == 1U);
    assert(transport.requests.front().url ==
           "https://api.themoviedb.org/3/configuration");
    assert(transport.requests.front().bearerToken ==
           "test.token_value-123");

    assert(scalar(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache "
        "WHERE backend_id='default' AND event_id='missing-series';") == 0);
    assert(scalar(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache "
        "WHERE backend_id='default' AND event_id='pictured-series';") == 1);
    assert(scalar(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache "
        "WHERE backend_id='default' AND event_id='missing-movie';") == 1);
    assert(scalar(
        database,
        "SELECT COUNT(*) FROM epg_scraper_metadata_cache "
        "WHERE backend_id='house-b' AND event_id='missing-series';") == 1);

    const std::filesystem::path tokenPath =
        root / "secrets" / "default.tmdb-token";
    assert(std::filesystem::is_regular_file(tokenPath));
    assert(fileMode(tokenPath) == 0600);
    std::ifstream tokenFile(tokenPath, std::ios::binary);
    const std::string storedToken(
        (std::istreambuf_iterator<char>(tokenFile)),
        std::istreambuf_iterator<char>());
    assert(storedToken == "test.token_value-123");

    transport.responses = {
        jsonResponse(
            "{\"backdrops\":[{"
            "\"file_path\":\"/regional-series.jpg\","
            "\"width\":1920,\"height\":1080,"
            "\"vote_average\":8.5,\"iso_639_1\":\"de\"}]}"),
        imageResponse("jpeg bytes")
    };

    EpgScraperMetadata metadata;
    metadata.backendId = "default";
    metadata.channelId = "channel";
    metadata.eventId = "event";
    metadata.provider = "tvscraper";
    metadata.mediaType = EpgScraperMediaType::Series;
    metadata.providerId = 108148;

    VdrEvent event;
    event.id = "event";
    event.channelId = "channel";

    const SeriesArtworkFallbackResolution artwork =
        service.resolve("default", event, metadata);
    assert(artwork.valid());
    assert(artwork.artwork.provider == "tmdb");
    assert(artwork.artwork.width == 1920);
    assert(artwork.artwork.height == 1080);
    assert(transport.requests.size() == 3U);
    assert(transport.requests[1].url.find("/tv/108148/images?") !=
           std::string::npos);
    assert(transport.requests[1].url.find("/find/") ==
           std::string::npos);
    assert(transport.requests[1].bearerToken ==
           "test.token_value-123");
    assert(transport.requests[2].bearerToken.empty());

    SeriesArtworkBackendSettingsUpdate forbiddenClear;
    forbiddenClear.backendId = "default";
    forbiddenClear.provider = "tmdb";
    forbiddenClear.clearTmdbReadAccessToken = true;
    const auto forbidden = service.update(forbiddenClear);
    assert(!forbidden.success);
    assert(forbidden.statusCode == 400);
    assert(forbidden.errorCode == "tmdb_token_required");
    assert(std::filesystem::is_regular_file(tokenPath));

    SeriesArtworkBackendSettingsUpdate switchToTvmaze;
    switchToTvmaze.backendId = "default";
    switchToTvmaze.provider = "tvmaze";
    switchToTvmaze.clearTmdbReadAccessToken = true;
    const auto switched = service.update(switchToTvmaze);
    assert(switched.success);
    assert(switched.settings.provider == "tvmaze");
    assert(!switched.settings.tmdbTokenConfigured);
    assert(!std::filesystem::exists(tokenPath));

    SeriesArtworkBackendSettingsUpdate invalid;
    invalid.backendId = "../default";
    invalid.provider = "tmdb";
    const auto rejected = service.update(invalid);
    assert(!rejected.success);
    assert(rejected.statusCode == 400);
    assert(rejected.errorCode == "invalid_backend_id");

    SeriesArtworkBackendSettingsUpdate nonAscii;
    nonAscii.backendId = "d\xc3\xa9fault";
    nonAscii.provider = "tvmaze";
    const auto nonAsciiRejected = service.update(nonAscii);
    assert(!nonAsciiRejected.success);
    assert(nonAsciiRejected.errorCode == "invalid_backend_id");

    std::error_code error;
    std::filesystem::remove_all(root, error);
    return 0;
}
