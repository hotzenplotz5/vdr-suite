#pragma once

#include "IExternalArtworkHttpTransport.h"

#include <set>
#include <string>

struct CurlExternalArtworkHttpTransportConfig
{
    std::set<std::string> allowedHosts = {
        "api.themoviedb.org",
        "api.tvmaze.com",
        "image.tmdb.org",
        "static.tvmaze.com"
    };
    std::string userAgent = "vdr-suite/epg-series-artwork";
};

class CurlExternalArtworkHttpTransport final
    : public IExternalArtworkHttpTransport
{
public:
    explicit CurlExternalArtworkHttpTransport(
        CurlExternalArtworkHttpTransportConfig config = {});

    ExternalArtworkHttpResponse perform(
        const ExternalArtworkHttpRequest& request) override;

    static bool isAllowedHttpsUrl(
        const std::string& url,
        const std::set<std::string>& allowedHosts);

private:
    CurlExternalArtworkHttpTransportConfig config_;
};
