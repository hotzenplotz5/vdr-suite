#include "RestfulApiSearchTimerDiscoveryProvider.h"

#include "HttpRequest.h"
#include "HttpResponse.h"
#include "IHttpClient.h"

#include <cassert>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
class SequencedHttpClient final : public IHttpClient
{
public:
    explicit SequencedHttpClient(
        std::vector<HttpResponse> responses)
        : responses_(std::move(responses))
    {
    }

    HttpResponse execute(
        const HttpRequest& request) const override
    {
        requests_.push_back(request);
        assert(nextResponse_ < responses_.size());
        return responses_[nextResponse_++];
    }

    const std::vector<HttpRequest>& requests() const
    {
        return requests_;
    }

private:
    std::vector<HttpResponse> responses_;
    mutable std::vector<HttpRequest> requests_;
    mutable std::size_t nextResponse_ = 0;
};

HttpResponse jsonResponse(
    const std::string& body)
{
    HttpResponse response;
    response.statusCode = 200;
    response.body = body;
    return response;
}
}

int main()
{
    RestfulApiSearchTimerDiscoveryProvider provider("default");

    assert(provider.configuredBackendId() == "default");
    assert(provider.discoveryEndpoint() == "/searchtimers/discovery.json");
    assert(provider.basePath().empty());

    SearchTimerDiscoveryCatalog defaultCatalog =
        provider.discover("");

    assert(defaultCatalog.backendId() == "default");
    assert(defaultCatalog.empty());
    assert(defaultCatalog.extendedEpgInfoCount() == 0);
    assert(defaultCatalog.channelGroupCount() == 0);
    assert(defaultCatalog.blacklistCount() == 0);
    assert(defaultCatalog.recordingDirectoryCount() == 0);

    SearchTimerDiscoveryCatalog remoteCatalog =
        provider.discover("ferienhaus");

    assert(remoteCatalog.backendId() == "ferienhaus");
    assert(remoteCatalog.empty());

    RestfulApiSearchTimerDiscoveryProvider customProvider(
        "living-room",
        "/custom/searchtimers/discovery.json");

    assert(customProvider.configuredBackendId() == "living-room");
    assert(customProvider.discoveryEndpoint() == "/custom/searchtimers/discovery.json");

    SearchTimerDiscoveryCatalog customCatalog =
        customProvider.discover("");

    assert(customCatalog.backendId() == "living-room");
    assert(customCatalog.empty());

    SequencedHttpClient httpClient({
        jsonResponse("{\"groups\":[\"News\",\"Movies\"],\"count\":2,\"total\":2}"),
        jsonResponse("{\"dirs\":[\"Doku\",\"Doku~Amerika~1929 - Der gro\\u00dfe B\\u00f6rsencrash\"],\"count\":2,\"total\":2}"),
        jsonResponse("{\"blacklists\":[{\"id\":3,\"search\":\"Sport\"},{\"id\":7,\"search\":\"B\\u00f6rse\"}],\"count\":2,\"total\":2}"),
        jsonResponse("{\"ext_epg_info\":[{\"id\":1,\"name\":\"category\",\"values\":[\"movie\",\"f\\u00fcr Kinder\"],\"config\":\"category|movie,f\\u00fcr Kinder\"}],\"count\":1,\"total\":1}")
    });

    RestfulApiSearchTimerDiscoveryProvider realProvider(
        "default",
        httpClient,
        "/api");

    SearchTimerDiscoveryCatalog realCatalog =
        realProvider.discover("home-vdr");

    assert(realCatalog.backendId() == "home-vdr");
    assert(realCatalog.channelGroupCount() == 2);
    assert(realCatalog.channelGroups().at(0).name() == "News");
    assert(realCatalog.channelGroups().at(1).name() == "Movies");
    assert(realCatalog.recordingDirectoryCount() == 2);
    assert(realCatalog.recordingDirectories().at(0).path() == "Doku");
    assert(realCatalog.recordingDirectories().at(1).path() ==
           "Doku~Amerika~1929 - Der große Börsencrash");
    assert(realCatalog.blacklistCount() == 2);
    assert(realCatalog.blacklists().at(0).id() == 3);
    assert(realCatalog.blacklists().at(0).search() == "Sport");
    assert(realCatalog.blacklists().at(1).id() == 7);
    assert(realCatalog.blacklists().at(1).search() == "Börse");
    assert(realCatalog.extendedEpgInfoCount() == 1);
    assert(realCatalog.extendedEpgInfos().at(0).id() == 1);
    assert(realCatalog.extendedEpgInfos().at(0).name() == "category");
    assert(realCatalog.extendedEpgInfos().at(0).values().size() == 2);
    assert(realCatalog.extendedEpgInfos().at(0).values().at(0) == "movie");
    assert(realCatalog.extendedEpgInfos().at(0).values().at(1) == "für Kinder");
    assert(realCatalog.extendedEpgInfos().at(0).config() == "category|movie,für Kinder");

    assert(httpClient.requests().size() == 4);
    assert(httpClient.requests().at(0).method == "GET");
    assert(httpClient.requests().at(0).url == "/api/searchtimers/channelgroups.json");
    assert(httpClient.requests().at(1).url == "/api/searchtimers/recordingdirs.json");
    assert(httpClient.requests().at(2).url == "/api/searchtimers/blacklists.json");
    assert(httpClient.requests().at(3).url == "/api/searchtimers/extepginfo.json");
    assert(httpClient.requests().at(0).headers.at("Accept") == "application/json");

    std::cout
        << "test_restfulapi_search_timer_discovery_provider_contract passed"
        << std::endl;

    return 0;
}
