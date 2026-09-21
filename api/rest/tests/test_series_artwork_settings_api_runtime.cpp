#include "Database.h"
#include "SeriesArtworkBackendSettingsService.h"
#include "SeriesArtworkSettingsApiRuntime.h"

#include <cassert>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace
{
class FakeTransport final : public IExternalArtworkHttpTransport
{
public:
    ExternalArtworkHttpResponse perform(
        const ExternalArtworkHttpRequest& request) override
    {
        ExternalArtworkHttpResponse response;
        response.attempted = true;

        if (request.url.find(
                "image.tmdb.org") !=
            std::string::npos)
        {
            response.statusCode = 200;
            response.contentType = "image/jpeg";
            response.body =
                "fake-series-cover-jpeg";
            return response;
        }

        response.statusCode = 500;
        response.transportError = true;
        return response;
    }
};

class FakeCache final : public ISeriesArtworkProviderCache
{
public:
    SeriesArtworkProviderCacheEntry find(
        const SeriesArtworkProviderCacheKey&,
        long long) override
    {
        return {};
    }

    bool store(
        const SeriesArtworkProviderCacheKey&,
        SeriesArtworkProviderCacheOutcome,
        long long) override
    {
        return true;
    }

    bool remove(
        const SeriesArtworkProviderCacheKey&) override
    {
        return true;
    }
};

std::filesystem::path tempRoot()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-series-artwork-api-runtime-test";

    std::error_code error;
    std::filesystem::remove_all(root, error);
    std::filesystem::create_directories(root / "secrets");

    std::filesystem::permissions(
        root / "secrets",
        std::filesystem::perms::owner_all,
        std::filesystem::perm_options::replace);

    return root;
}
}

int main()
{
    const std::filesystem::path root = tempRoot();

    Database database;
    assert(database.open((root / "settings.db").string()));

    FakeTransport transport;
    FakeCache cache;

    SeriesArtworkBackendSettingsConfig config;
    config.defaultProvider = "none";
    config.secretRoot = (root / "secrets").string();
    config.seriesCoverCacheRoot =
        (root / "series-covers").string();
    config.environmentTmdbReadAccessToken =
        "test-read-access-token";

    SeriesArtworkBackendSettingsService service(
        database,
        transport,
        cache,
        config);

    assert(service.ensureSchema());

    SeriesArtworkSettingsApiRuntime& runtime =
        SeriesArtworkSettingsApiRuntime::instance();

    runtime.reset();
    runtime.registerBackend("default", service);

    ApiResponse response;

    assert(runtime.tryHandleGet(
        "/api/backends/default/settings/series-artwork",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"coverOverrides\":[]") !=
           std::string::npos);

    const std::string seriesKey =
        "folder:serien/testserie";
    const std::string posterUrl =
        "/api/vdr/recordings/metadata/image?"
        "backend=default&backendNativeId=native-episode&kind=gallery&index=2";

    const std::string setBody =
        "{"
        "\"backendId\":\"default\","
        "\"operation\":\"set-series-cover\","
        "\"seriesKey\":\"" + seriesKey + "\","
        "\"posterUrl\":\"" + posterUrl + "\""
        "}";

    assert(runtime.tryHandlePost(
        "/api/backends/default/settings/series-artwork",
        setBody,
        response));

    assert(response.statusCode == 200);
    assert(response.body.find("\"coverOverrides\":[{") !=
           std::string::npos);
    assert(response.body.find("\"seriesKey\":\"" + seriesKey + "\"") !=
           std::string::npos);
    assert(response.body.find("\"posterUrl\":\"" + posterUrl + "\"") !=
           std::string::npos);
    assert(response.body.find("\"revision\":1") !=
           std::string::npos);

    assert(runtime.tryHandleGet(
        "/api/backends/default/settings/series-artwork",
        response));
    assert(response.statusCode == 200);
    assert(response.body.find("\"seriesKey\":\"" + seriesKey + "\"") !=
           std::string::npos);

    const std::string clearBody =
        "{"
        "\"backendId\":\"default\","
        "\"operation\":\"clear-series-cover\","
        "\"seriesKey\":\"" + seriesKey + "\""
        "}";

    assert(runtime.tryHandlePost(
        "/api/backends/default/settings/series-artwork",
        clearBody,
        response));

    assert(response.statusCode == 200);
    assert(response.body.find("\"coverOverrides\":[]") !=
           std::string::npos);

    const std::string tmdbBody =
        "{"
        "\"backendId\":\"default\","
        "\"operation\":\"set-series-cover-tmdb\","
        "\"seriesKey\":\"folder:serien/testserie\","
        "\"providerId\":\"tmdb\","
        "\"externalNamespace\":\"tv\","
        "\"externalId\":\"1396\","
        "\"posterReference\":\"/series-test.jpg\""
        "}";

    assert(runtime.tryHandlePost(
        "/api/backends/default/settings/series-artwork",
        tmdbBody,
        response));

    assert(response.statusCode == 200);

    assert(
        response.body.find(
            "/settings/series-artwork/image?seriesKey=") !=
        std::string::npos &&
        "TMDB Series cover must materialize independently of Recording assignment");

    assert(runtime.tryHandleGet(
        "/api/backends/default/settings/series-artwork/"
        "candidate-image?"
        "externalId=1396&"
        "posterReference=%2Fseries-test.jpg",
        response));

    assert(response.statusCode == 200);
    assert(response.contentType == "image/jpeg");
    assert(response.body == "fake-series-cover-jpeg");

    assert(runtime.tryHandleGet(
        "/api/backends/default/settings/series-artwork/image?"
        "seriesKey=folder%3Aserien%2Ftestserie",
        response));

    assert(response.statusCode == 200);
    assert(response.contentType == "image/jpeg");
    assert(response.body == "fake-series-cover-jpeg");

    const std::string externalBody =
        "{"
        "\"backendId\":\"default\","
        "\"operation\":\"set-series-cover\","
        "\"seriesKey\":\"folder:serien/testserie\","
        "\"posterUrl\":\"https://example.invalid/poster.jpg\""
        "}";

    assert(runtime.tryHandlePost(
        "/api/backends/default/settings/series-artwork",
        externalBody,
        response));

    assert(response.statusCode == 400);
    assert(response.body.find("invalid_series_cover_url") !=
           std::string::npos);

    runtime.reset();

    std::error_code error;
    std::filesystem::remove_all(root, error);

    return 0;
}
