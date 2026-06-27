#pragma once

#include "ISearchTimerDiscoveryProvider.h"

#include <string>

class IHttpClient;

class RestfulApiSearchTimerDiscoveryProvider final
    : public ISearchTimerDiscoveryProvider
{
public:
    explicit RestfulApiSearchTimerDiscoveryProvider(
        std::string configuredBackendId,
        std::string discoveryEndpoint = "/searchtimers/discovery.json");

    RestfulApiSearchTimerDiscoveryProvider(
        std::string configuredBackendId,
        IHttpClient& httpClient,
        std::string basePath = "",
        std::string discoveryEndpoint = "/searchtimers/discovery.json");

    const std::string& configuredBackendId() const;
    const std::string& discoveryEndpoint() const;
    const std::string& basePath() const;

    SearchTimerDiscoveryCatalog discover(
        const std::string& backendId) const override;

private:
    std::string configuredBackendId_;
    std::string discoveryEndpoint_;
    std::string basePath_;
    IHttpClient* httpClient_ = nullptr;
};
