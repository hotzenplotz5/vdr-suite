#include "IExternalArtworkHttpTransport.h"
#include "TmdbRecordingMetadataCandidateProvider.h"

#include <cassert>
#include <filesystem>
#include <fstream>
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

std::string readAll(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    return std::string(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}
}

int main()
{
    const std::filesystem::path root =
        std::filesystem::temp_directory_path() /
        "vdr-suite-manual-recording-poster-test";
    std::error_code error;
    std::filesystem::remove_all(root, error);

    FakeTransport transport;
    ExternalArtworkHttpResponse image;
    image.attempted = true;
    image.statusCode = 200;
    image.contentType = "image/jpeg";
    image.body = "fake-jpeg-payload";
    transport.responses.push_back(image);

    TmdbRecordingMetadataCandidateProviderConfig config;
    config.readAccessToken = "test-token";
    config.posterCacheRoot = root.string();
    config.maximumImageBytes = 1024U * 1024U;
    TmdbRecordingMetadataCandidateProvider provider(transport, config);

    const std::string materialized = provider.materializePoster(
        "movie",
        "13",
        "/forrest.jpg");
    assert(!materialized.empty());
    assert(std::filesystem::is_regular_file(materialized));
    assert(readAll(materialized) == "fake-jpeg-payload");
    assert(transport.requests.size() == 1U);
    assert(transport.requests.front().url ==
        "https://image.tmdb.org/t/p/w500/forrest.jpg");
    assert(transport.requests.front().bearerToken.empty());
    assert(transport.requests.front().maximumResponseBytes ==
        config.maximumImageBytes);

    const std::string cached = provider.materializePoster(
        "movie",
        "13",
        "/forrest.jpg");
    assert(cached == materialized);
    assert(transport.requests.size() == 1U);

    assert(provider.materializePoster(
        "movie", "not-a-number", "/forrest.jpg").empty());
    assert(provider.materializePoster(
        "unknown", "13", "/forrest.jpg").empty());
    assert(provider.materializePoster(
        "movie", "13", "https://example.test/image.jpg").empty());
    assert(provider.materializePoster(
        "movie", "13", "/../secret.jpg").empty());

    std::filesystem::remove_all(root, error);
    return 0;
}
